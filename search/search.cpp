#include "search.h"
#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include "../board/types.h"
#include "../board/bitboard.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace chess {

// Piece values for Static Exchange Evaluation
const int Search::SEE_VALUES[7] = {0, 100, 320, 330, 500, 900, 20000};

Search::Search(Evaluator* eval) : evaluator(eval), tablebase(nullptr), time_limit_ms(0), stop_search(false),
                                   multi_pv(1), use_null_move(true), use_lmr(true), use_futility(true) {
    tt_ = std::make_unique<TranspositionTable>();
    std::memset(history, 0, sizeof(history));
    std::memset(killers, 0, sizeof(killers));
    std::memset(counter_moves, 0, sizeof(counter_moves));
}

void Search::set_hash_size(size_t mb) {
    if (tt_) tt_->resize(mb);
}

Search::~Search() {
}

uint64_t get_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Search::time_over() const {
    if (time_limit_ms == 0) return false;
    return (get_time_ms() - start_time) >= time_limit_ms;
}

Search::TTProbeResult Search::probe_tt(uint64_t key, int depth, int alpha, int beta, int ply) {
    TTProbeResult result = {false, false, 0, MOVE_NONE};
    if (!tt_) return result;
    
    int score_val;
    Move move_val;
    TTFlag flag_val;
    bool usable = tt_->probe(key, depth, alpha, beta, score_val, move_val, flag_val);
    
    // On key match we get move_val (probe fills it before returning)
    result.found = (move_val != MOVE_NONE || usable);
    result.move = move_val;
    
    if (!usable) return result;
    
    // Adjust mate scores from storage (root-relative to ply-relative)
    int score = score_val;
    if (score > 29000) score -= ply;
    else if (score < -29000) score += ply;
    
    result.usable = true;
    result.score = score;
    return result;
}

void Search::store_tt(uint64_t key, int depth, int score, Move move, int flag, int ply) {
    if (!tt_) return;
    
    // Adjust mate scores for storage (ply-relative to root-relative)
    int store_score = score;
    if (score > 29000) store_score = score + ply;
    else if (score < -29000) store_score = score - ply;
    
    TTFlag tt_flag = (flag == 0) ? TT_EXACT : (flag == 1) ? TT_ALPHA : TT_BETA;
    tt_->store(key, depth, store_score, move, tt_flag);
}

void Search::order_moves(const Position& pos, std::vector<Move>& moves, Move tt_move, 
                         int ply, Move counter_move) {
    // Advanced move ordering with multiple heuristics
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        int score_a = 0, score_b = 0;
        
        // 1. TT move gets highest priority
        if (a.data == tt_move.data) return true;
        if (b.data == tt_move.data) return false;
        
        // 2. Promotions (especially queen promotions) - very high priority
        if (a.is_promotion()) {
            score_a += 11000000;
            if (a.promotion() == QUEEN) score_a += 1000000;
        }
        if (b.is_promotion()) {
            score_b += 11000000;
            if (b.promotion() == QUEEN) score_b += 1000000;
        }
        
        // 3. Winning captures (SEE > 0)
        if (a.is_capture()) {
            int see_a = see(pos, a);
            if (see_a > 0) score_a += 10000000 + see_a;
            else score_a += 5000000 + see_a; // Losing captures still before quiets
        }
        if (b.is_capture()) {
            int see_b = see(pos, b);
            if (see_b > 0) score_b += 10000000 + see_b;
            else score_b += 5000000 + see_b;
        }
        
        // 4. Killer moves (non-captures that caused beta cutoffs)
        if (!a.is_capture() && ply < MAX_PLY) {
            if (a.data == killers[ply][0].data) score_a += 8000000;
            else if (a.data == killers[ply][1].data) score_a += 7000000;
        }
        if (!b.is_capture() && ply < MAX_PLY) {
            if (b.data == killers[ply][0].data) score_b += 8000000;
            else if (b.data == killers[ply][1].data) score_b += 7000000;
        }
        
        // 5. Counter move heuristic
        if (!a.is_capture() && a.data == counter_move.data) score_a += 6000000;
        if (!b.is_capture() && b.data == counter_move.data) score_b += 6000000;
        
        // 6. History heuristic (for quiet moves)
        if (!a.is_capture()) {
            score_a += history[pos.side_to_move()][a.from()][a.to()];
        }
        if (!b.is_capture()) {
            score_b += history[pos.side_to_move()][b.from()][b.to()];
        }
        
        return score_a > score_b;
    });
}

// Full Static Exchange Evaluation - simulates the capture sequence
int Search::see(const Position& pos, Move move) const {
    Square to = move.to();
    Square from = move.from();
    
    Piece captured = pos.piece_on(to);
    if (captured == NO_PIECE_PIECE && !move.is_en_passant()) {
        return 0;
    }
    
    int gain = SEE_VALUES[type_of(captured)];
    if (move.is_promotion()) {
        gain -= SEE_VALUES[PAWN];
        gain += SEE_VALUES[move.promotion()];
    }
    if (move.is_en_passant()) {
        gain = SEE_VALUES[PAWN];
    }
    
    Position pos_copy(pos);
    UndoInfo undo;
    Piece attacker = pos.piece_on(from);
    PieceType attacker_pt = type_of(attacker);
    
    PieceType promo = move.is_promotion() ? move.promotion() : NO_PIECE;
    if (attacker_pt == PAWN && (rank_of(to) == 0 || rank_of(to) == 7) && promo == NO_PIECE) {
        promo = QUEEN;
    }
    Move capture_move(from, to, promo, true, move.is_en_passant());
    
    if (!pos_copy.make_move(capture_move, undo)) {
        return gain;
    }
    
    int recapture_val = see_recursive(pos_copy, to, 1);
    return std::max(0, gain - recapture_val);
}

int Search::see_recursive(Position& pos, Square to_sq, int depth) const {
    if (depth > 16) return 0;
    
    Color side = pos.side_to_move();
    Bitboard attackers = Attacks::attackers_to(pos, to_sq, side);
    if (!attackers) return 0;
    
    constexpr PieceType order[] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
    Square from_sq = SQ_NONE;
    PieceType pt = NO_PIECE;
    
    for (PieceType p : order) {
        Bitboard bb = attackers & pos.pieces(side, p);
        if (bb) {
            from_sq = static_cast<Square>(lsb(bb));
            pt = p;
            break;
        }
    }
    
    if (from_sq == SQ_NONE || pt == NO_PIECE) return 0;
    
    Piece captured = pos.piece_on(to_sq);
    int gain = SEE_VALUES[type_of(captured)];
    
    PieceType promo = NO_PIECE;
    if (pt == PAWN && (rank_of(to_sq) == 0 || rank_of(to_sq) == 7)) {
        promo = QUEEN;
        gain += SEE_VALUES[QUEEN] - SEE_VALUES[PAWN];
    }
    
    Move capture_move(from_sq, to_sq, promo, true, false);
    UndoInfo undo;
    if (!pos.make_move(capture_move, undo)) return gain;
    
    int recapture_val = see_recursive(pos, to_sq, depth + 1);
    pos.unmake_move(capture_move, undo);
    
    return std::max(0, gain - recapture_val);
}

int Search::quiescence(Position& pos, int alpha, int beta) {
    stats.qnodes++;
    
    // Limit quiescence depth to prevent infinite loops
    static thread_local int qdepth = 0;
    if (qdepth > 10) return evaluator->evaluate(pos);  // Max quiescence depth
    qdepth++;
    
    if (time_over()) {
        stop_search = true;
        qdepth--;
        return alpha;
    }
    
    bool in_check = Attacks::in_check(pos, pos.side_to_move());
    
    // Stand pat - but not if we're in check (must search all moves)
    int stand_pat = evaluator->evaluate(pos);
    if (!in_check) {
        if (stand_pat >= beta) {
            qdepth--;
            return beta;
        }
        if (stand_pat > alpha) alpha = stand_pat;
    }
    
    // Generate captures (and checks if in check or at low qdepth)
    std::vector<Move> moves;
    if (in_check) {
        // If in check, must search all legal moves
        MoveGen::generate_legal(pos, moves);
        if (moves.empty()) {
            // Checkmate
            qdepth--;
            return -30000 + qdepth;
        }
    } else {
        MoveGen::generate_captures(pos, moves);
        // Add moves that give check (important for tactical sequences)
        std::vector<Move> all_moves;
        MoveGen::generate_legal(pos, all_moves);
        for (const Move& m : all_moves) {
            if (!m.is_capture()) {
                UndoInfo undo;
                if (pos.make_move(m, undo)) {
                    if (Attacks::in_check(pos, pos.side_to_move())) {
                        moves.push_back(m);
                    }
                    pos.unmake_move(m, undo);
                }
            }
        }
        
        if (moves.empty()) {
            qdepth--;
            return stand_pat;
        }
    }
    
    order_moves(pos, moves, MOVE_NONE, 0, MOVE_NONE);
    
    for (Move move : moves) {
        // Delta pruning - skip captures that can't improve alpha
        if (!in_check && !move.is_promotion()) {
            int delta = 900; // Queen value
            if (move.is_capture()) {
                Piece captured = pos.piece_on(move.to());
                delta = SEE_VALUES[type_of(captured)];
            }
            if (stand_pat + delta + 200 < alpha) {
                continue; // This capture can't possibly raise alpha
            }
        }
        
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        
        int score = -quiescence(pos, -beta, -alpha);
        pos.unmake_move(move, undo);
        
        if (stop_search) {
            qdepth--;
            return alpha;
        }
        
        if (score >= beta) {
            qdepth--;
            return beta;
        }
        if (score > alpha) alpha = score;
    }
    
    qdepth--;
    return alpha;
}

int Search::alpha_beta(Position& pos, int depth, int ply, int alpha, int beta, 
                       bool null_move_allowed, std::vector<Move>& pv, Move prev_move) {
    stats.nodes++;
    pv.clear();
    
    // Safety check - prevent infinite recursion
    if (ply >= MAX_PLY || stats.nodes > 10000000) {
        stop_search = true;
        return evaluator->evaluate(pos);
    }
    
    if (time_over()) {
        stop_search = true;
        return alpha;
    }
    
    // Check for draw
    if (pos.is_draw()) return 0;
    
    // Probe tablebase if enabled and at sufficient depth
    if (tablebase && tablebase->is_initialized() && depth >= tablebase->get_probe_depth()) {
        int tb_score;
        if (tablebase->probe_search(pos, tb_score)) {
            return tb_score;
        }
    }
    
    bool in_check = Attacks::in_check(pos, pos.side_to_move());
    
    // Check extension - search deeper if in check
    if (in_check) depth++;
    
    // Quiescence at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta);
    }
    
    // Transposition table probe
    uint64_t key = pos.zobrist_key();
    TTProbeResult tt_result = probe_tt(key, depth, alpha, beta, ply);
    if (tt_result.usable && ply > 0) {
        return tt_result.score;
    }
    
    // Check for checkmate/stalemate
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    if (moves.empty()) {
        if (in_check) {
            return -30000 + ply; // Checkmate (prefer shorter mates)
        }
        return 0; // Stalemate
    }
    
    int static_eval = evaluator->evaluate(pos);
    
    // Null Move Pruning
    // Try giving opponent a free move - if we still beat beta, position is too good
    if (use_null_move && null_move_allowed && !in_check && depth >= NULL_MOVE_MIN_DEPTH) {
        // Don't do null move in endgame positions with only pawns and king
        bool has_pieces = false;
        for (PieceType pt = KNIGHT; pt <= QUEEN; pt = static_cast<PieceType>(pt + 1)) {
            if (pos.pieces(pos.side_to_move(), pt)) {
                has_pieces = true;
                break;
            }
        }
        
        if (has_pieces && static_eval >= beta) {
            UndoInfo null_undo;
            pos.make_null_move(null_undo);
            std::vector<Move> null_pv;
            int null_score = -alpha_beta(pos, depth - 1 - NULL_MOVE_R, ply + 1, -beta, -beta + 1, false, null_pv, MOVE_NONE);
            pos.unmake_null_move(null_undo);
            
            if (null_score >= beta) {
                stats.null_move_cutoffs++;
                if (depth > 6) {
                    std::vector<Move> verify_pv;
                    int verify = alpha_beta(pos, depth - NULL_MOVE_R, ply, beta - 1, beta, false, verify_pv, prev_move);
                    if (verify >= beta) return beta;
                } else {
                    return beta;
                }
            }
        }
    }
    
    // Razoring - if eval is far below alpha at low depth, go straight to qsearch
    if (!in_check && depth <= 3 && use_futility) {
        int razor_margin = RAZOR_MARGIN * depth;
        if (static_eval + razor_margin < alpha) {
            int q_score = quiescence(pos, alpha - razor_margin, alpha - razor_margin + 1);
            if (q_score + razor_margin < alpha) {
                return q_score;
            }
        }
    }
    
    // Futility pruning - at low depths, prune moves unlikely to raise alpha
    bool futility_prune = false;
    if (!in_check && depth <= 3 && use_futility && static_eval + FUTILITY_MARGIN * depth < alpha) {
        futility_prune = true;
    }
    
    Move best_move = MOVE_NONE;
    int best_score = -30000;
    int flag = 1; // Alpha flag
    std::vector<Move> best_pv;
    
    Move tt_move = tt_result.found ? tt_result.move : MOVE_NONE;
    Move counter_move = MOVE_NONE;
    Piece prev_piece = NO_PIECE_PIECE;
    if (prev_move != MOVE_NONE && prev_move.is_valid()) {
        prev_piece = pos.piece_on(prev_move.to());
        if (prev_piece != NO_PIECE_PIECE && prev_piece < 16) {
            counter_move = counter_moves[prev_piece][prev_move.to()];
        }
    }
    order_moves(pos, moves, tt_move, ply, counter_move);
    
    int legal_moves = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        legal_moves++;
        
        // Futility pruning - skip quiet moves in losing positions at low depth
        if (futility_prune && !move.is_capture() && !move.is_promotion() && 
            !Attacks::in_check(pos, pos.side_to_move())) {
            pos.unmake_move(move, undo);
            stats.futility_prunes++;
            continue;
        }
        
        int score = -30000;
        std::vector<Move> child_pv;
        
        // Late Move Reduction (LMR)
        // Reduce search depth for later moves that are less likely to be best
        bool do_lmr = use_lmr && legal_moves >= LMR_MIN_MOVES && depth >= LMR_MIN_DEPTH && 
                      !in_check && !move.is_capture() && !move.is_promotion() &&
                      !Attacks::in_check(pos, pos.side_to_move());
        
        if (do_lmr) {
            // Calculate reduction based on move number and depth (improved formula)
            int reduction = 1;
            if (legal_moves > 8 && depth > 8) {
                reduction = 3;  // Very late moves at high depth
            } else if (legal_moves > 6 && depth > 6) {
                reduction = 2;  // Late moves at medium depth
            } else if (legal_moves > 4) {
                reduction = 1;  // Moderate reduction for somewhat late moves
            }
            
            // Search with reduced depth
            score = -alpha_beta(pos, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, true, child_pv, move);
            stats.lmr_searches++;
            
            // If reduced search beats alpha, re-search at full depth
            if (score > alpha) {
                score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
            }
        } else {
            // Principal Variation Search (PVS)
            if (legal_moves == 1) {
                // First move - search with full window
                score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
            } else {
                // Other moves - try null window search first
                score = -alpha_beta(pos, depth - 1, ply + 1, -alpha - 1, -alpha, true, child_pv, move);
                
                // If it beats alpha, re-search with full window
                if (score > alpha && score < beta) {
                    score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
                }
            }
        }
        
        pos.unmake_move(move, undo);
        
        if (stop_search) return alpha;
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
            
            // Update PV
            best_pv.clear();
            best_pv.push_back(move);
            best_pv.insert(best_pv.end(), child_pv.begin(), child_pv.end());
        }
        
        if (score >= beta) {
            // Beta cutoff - update heuristics
            if (!move.is_capture() && ply < MAX_PLY) {
                // Update killer moves
                if (killers[ply][0].data != move.data) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = move;
                }
                
                // Update history heuristic
                history[pos.side_to_move()][move.from()][move.to()] += depth * depth;
                
                // Cap history values to prevent overflow
                if (history[pos.side_to_move()][move.from()][move.to()] > 10000) {
                    // Age all history values
                    for (int c = 0; c < 2; c++) {
                        for (int f = 0; f < 64; f++) {
                            for (int t = 0; t < 64; t++) {
                                history[c][f][t] /= 2;
                            }
                        }
                    }
                }
            }
            
            store_tt(key, depth, beta, move, 2, ply); // Beta flag
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
            flag = 0; // Exact flag
            // Update counter move heuristic when we find a new best move
            if (best_move != MOVE_NONE && best_move.is_valid() && prev_move != MOVE_NONE && prev_move.is_valid() &&
                !best_move.is_capture() && prev_piece != NO_PIECE_PIECE && prev_piece < 16) {
                counter_moves[prev_piece][prev_move.to()] = best_move;
            }
        }
    }
    
    // Update PV
    pv = best_pv;
    
    store_tt(key, depth, best_score, best_move, flag, ply);
    return best_score;
}

Move Search::search(Position& pos, int depth, int time_limit_ms) {
    stats = SearchStats();
    stats.depth = depth;
    stop_search = false;
    this->time_limit_ms = time_limit_ms;
    start_time = get_time_ms();
    
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    if (moves.empty()) return MOVE_NONE;
    
    // Order moves to search best moves first
    uint64_t key = pos.zobrist_key();
    TTProbeResult tt_result = probe_tt(key, depth - 1, -30000, 30000, 0);
    Move tt_move = tt_result.found ? tt_result.move : MOVE_NONE;
    order_moves(pos, moves, tt_move, 0, MOVE_NONE);
    
    int alpha = -30000;
    int beta = 30000;
    Move best_move = moves[0];
    std::vector<Move> pv;
    
    for (Move move : moves) {
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        
        std::vector<Move> child_pv;
        int score = -alpha_beta(pos, depth - 1, 1, -beta, -alpha, true, child_pv, move);
        pos.unmake_move(move, undo);
        
        if (stop_search) break;
        
        if (score > alpha) {
            alpha = score;
            best_move = move;
            stats.best_score = score;
            pv.clear();
            pv.push_back(move);
            pv.insert(pv.end(), child_pv.begin(), child_pv.end());
        }
    }
    
    stats.best_move = best_move;
    stats.pv_lines[0] = pv;
    return best_move;
}

Move Search::iterative_deepening(Position& pos, int max_depth, int time_limit) {
    stats = SearchStats();
    stop_search = false;
    time_limit_ms = time_limit;
    start_time = get_time_ms();
    
    Move best_move = MOVE_NONE;
    int prev_score = 0;
    
    try {
        for (int depth = 1; depth <= max_depth && !stop_search; depth++) {
            stats.depth = depth;
            uint64_t nodes_before = stats.nodes;
            
            std::vector<Move> moves;
            MoveGen::generate_legal(pos, moves);
            if (moves.empty()) break;
            
            
            // Aspiration Windows - use narrow window around previous score
            int alpha = -30000;
            int beta = 30000;
            if (depth >= 5) {
                alpha = prev_score - ASPIRATION_WINDOW;
                beta = prev_score + ASPIRATION_WINDOW;
            } else {
                alpha = -30000;
                beta = 30000;
            }
            
            int aspiration_delta = ASPIRATION_WINDOW;
            int fail_high_count = 0;
            int fail_low_count = 0;
            
            // Aspiration window loop - widen and re-search if we fail
            while (true) {
                Move current_best = moves[0];
                int best_score = -30000;
                std::vector<Move> best_pv;
                
                // Save original window for fail-high/fail-low detection
                int original_alpha = alpha;
                int original_beta = beta;
                
                // Order moves based on previous iteration
                Move tt_move = MOVE_NONE;
                if (depth > 1) {
                    uint64_t key = pos.zobrist_key();
                    TTProbeResult tt_result = probe_tt(key, depth - 1, -30000, 30000, 0);
                    if (tt_result.found) {
                        tt_move = tt_result.move;
                    }
                }
                order_moves(pos, moves, tt_move, 0, MOVE_NONE);
                
                for (size_t i = 0; i < moves.size() && !stop_search; i++) {
                    Move move = moves[i];
                    UndoInfo undo;
                    if (!pos.make_move(move, undo)) continue;
                    
                    std::vector<Move> child_pv;
                    int score = -30000;
                    
                    if (i == 0) {
                        // First move - full window
                        score = -alpha_beta(pos, depth - 1, 1, -beta, -alpha, true, child_pv, move);
                    } else {
                        // PVS - null window search first
                        score = -alpha_beta(pos, depth - 1, 1, -alpha - 1, -alpha, true, child_pv, move);
                        if (score > alpha && score < beta) {
                            score = -alpha_beta(pos, depth - 1, 1, -beta, -alpha, true, child_pv, move);
                        }
                    }
                    
                    pos.unmake_move(move, undo);
                    
                    if (stop_search) break;
                    
                    if (score > best_score) {
                        best_score = score;
                        current_best = move;
                        
                        best_pv.clear();
                        best_pv.push_back(move);
                        best_pv.insert(best_pv.end(), child_pv.begin(), child_pv.end());
                    }
                    
                    if (score > alpha) {
                        alpha = score;
                    }
                }
                
                if (stop_search) break;
                
                // Check if we failed high or low (compare to ORIGINAL window)
                if (best_score <= original_alpha) {
                    // Failed low - widen window downward
                    alpha = std::max(-30000, original_alpha - aspiration_delta);
                    beta = original_beta;
                    aspiration_delta *= 2;
                    fail_low_count++;
                    if (fail_low_count > 3) {
                        alpha = -30000;
                        beta = 30000;
                    }
                    continue;
                } else if (best_score >= original_beta) {
                    // Failed high - widen window upward
                    alpha = original_alpha;
                    beta = std::min(30000, original_beta + aspiration_delta);
                    aspiration_delta *= 2;
                    fail_high_count++;
                    if (fail_high_count > 3) {
                        alpha = -30000;
                        beta = 30000;
                    }
                    continue;
                }
                
                // Success - score is within window
                best_move = current_best;
                stats.best_move = best_move;
                stats.best_score = best_score;
                stats.pv_lines[0] = best_pv;
                prev_score = best_score;
                
                // Output info for this depth (UCI format)
                std::ostringstream pv_str;
                for (size_t i = 0; i < best_pv.size() && i < 10; i++) {
                    if (i > 0) pv_str << " ";
                    Square from = best_pv[i].from();
                    Square to = best_pv[i].to();
                    pv_str << char('a' + file_of(from)) << char('1' + rank_of(from))
                           << char('a' + file_of(to)) << char('1' + rank_of(to));
                    if (best_pv[i].is_promotion()) {
                        switch (best_pv[i].promotion()) {
                            case KNIGHT: pv_str << 'n'; break;
                            case BISHOP: pv_str << 'b'; break;
                            case ROOK: pv_str << 'r'; break;
                            case QUEEN: pv_str << 'q'; break;
                            default: break;
                        }
                    }
                }
                
                uint64_t nodes_this_depth = stats.nodes - nodes_before;
                uint64_t time_elapsed = get_time_ms() - start_time;
                uint64_t nps = (time_elapsed > 0) ? (stats.nodes * 1000 / time_elapsed) : 0;
                
                std::cout << "info depth " << depth 
                          << " seldepth " << depth
                          << " score cp " << best_score 
                          << " nodes " << stats.nodes
                          << " nps " << nps
                          << " time " << time_elapsed
                          << " pv " << pv_str.str() << std::endl;
                std::cout.flush();
                
                break; // Success, exit aspiration loop
            }
        }
    } catch (...) {
        // Catch any exceptions and return best move found so far
        std::cerr << "Exception in search!" << std::endl;
    }
    
    return best_move;
}

void Search::clear_tt() {
    if (tt_) tt_->clear();
    std::memset(history, 0, sizeof(history));
    std::memset(killers, 0, sizeof(killers));
    std::memset(counter_moves, 0, sizeof(counter_moves));
}

} // namespace chess

