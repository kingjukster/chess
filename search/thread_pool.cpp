#include "thread_pool.h"
#include "../eval/evaluator.h"
#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace chess {

static uint64_t get_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Piece values for SEE
static const int SEE_VALUES[7] = {0, 100, 320, 330, 500, 900, 20000};

// Search constants
static const int NULL_MOVE_R = 2;
static const int NULL_MOVE_MIN_DEPTH = 3;
static const int LMR_MIN_DEPTH = 3;
static const int LMR_MIN_MOVES = 3;
static const int FUTILITY_MARGIN = 100;
static const int RAZOR_MARGIN = 300;
static const int ASPIRATION_WINDOW = 50;
static const int MAX_PLY = 256;

//-----------------------------------------------------------------------------
// ThreadPool implementation
//-----------------------------------------------------------------------------

ThreadPool::ThreadPool(Evaluator* eval, TranspositionTable* tt)
    : evaluator_(eval), tt_(tt), searching_(false), stop_(false), num_threads_(1) {
    // Create main thread
    threads_.push_back(std::make_unique<SearchThread>(0, eval, tt, this));
}

ThreadPool::~ThreadPool() {
    stop_search();
    threads_.clear();
}

void ThreadPool::set_threads(int count) {
    if (count < 1) count = 1;
    if (count > 256) count = 256;
    
    if (count == num_threads_) return;
    
    // Stop current search if running
    if (searching_) {
        stop_search();
        wait_for_search();
    }
    
    // Resize thread pool
    threads_.clear();
    for (int i = 0; i < count; i++) {
        threads_.push_back(std::make_unique<SearchThread>(i, evaluator_, tt_, this));
    }
    
    num_threads_ = count;
}

void ThreadPool::start_search(const Position& pos, const SearchLimits& limits) {
    stop_ = false;
    searching_ = true;
    
    // Start new search generation in TT
    tt_->new_search();
    
    // Start all threads
    for (auto& thread : threads_) {
        thread->start(pos, limits);
    }
}

void ThreadPool::stop_search() {
    stop_ = true;
    
    for (auto& thread : threads_) {
        thread->stop();
    }
}

void ThreadPool::wait_for_search() {
    for (auto& thread : threads_) {
        thread->wait();
    }
    searching_ = false;
}

Move ThreadPool::best_move() const {
    if (threads_.empty()) return MOVE_NONE;
    return threads_[0]->best_move();
}

void ThreadPool::get_statistics(SearchStatistics& stats) const {
    stats.reset();
    
    for (const auto& thread : threads_) {
        stats.nodes.store(stats.nodes.load() + thread->nodes());
        stats.qnodes.store(stats.qnodes.load() + thread->qnodes());
        stats.null_move_cutoffs.store(stats.null_move_cutoffs.load() + thread->null_move_cutoffs());
        stats.futility_prunes.store(stats.futility_prunes.load() + thread->futility_prunes());
        stats.lmr_searches.store(stats.lmr_searches.load() + thread->lmr_searches());
        stats.beta_cutoffs.store(stats.beta_cutoffs.load() + thread->beta_cutoffs());
        stats.first_move_cutoffs.store(stats.first_move_cutoffs.load() + thread->first_move_cutoffs());
        
        if (thread->seldepth() > stats.seldepth) {
            stats.seldepth = thread->seldepth();
        }
    }
    
    // Get best result from main thread
    if (!threads_.empty()) {
        stats.best_move = threads_[0]->best_move();
        stats.best_score = threads_[0]->best_score();
        stats.pv = threads_[0]->pv();
    }
}

void ThreadPool::start_pondering(const Position& pos, Move ponder_move) {
    SearchLimits limits;
    limits.infinite = true;
    limits.ponder = true;
    start_search(pos, limits);
}

void ThreadPool::ponderhit() {
    // Convert pondering to normal search
    // This would require updating limits in all threads
    // For now, just continue searching
}

//-----------------------------------------------------------------------------
// SearchThread implementation
//-----------------------------------------------------------------------------

SearchThread::SearchThread(int id, Evaluator* eval, TranspositionTable* tt, ThreadPool* pool)
    : id_(id), evaluator_(eval), tt_(tt), pool_(pool), searching_(false), stop_(false),
      best_move_(MOVE_NONE), best_score_(0), seldepth_(0),
      nodes_(0), qnodes_(0), null_move_cutoffs_(0), futility_prunes_(0),
      lmr_searches_(0), beta_cutoffs_(0), first_move_cutoffs_(0) {
    
    std::memset(history_, 0, sizeof(history_));
    std::memset(killers_, 0, sizeof(killers_));
    std::memset(counter_moves_, 0, sizeof(counter_moves_));
    
    // Vary aspiration window slightly per thread for Lazy SMP
    aspiration_delta_ = ASPIRATION_WINDOW + (id_ * 5);
}

SearchThread::~SearchThread() {
    stop();
    wait();
}

void SearchThread::start(const Position& pos, const SearchLimits& limits) {
    if (searching_) {
        stop();
        wait();
    }
    
    root_pos_ = pos;
    limits_ = limits;
    stop_ = false;
    searching_ = true;
    start_time_ = get_time_ms();
    
    // Reset statistics
    nodes_ = 0;
    qnodes_ = 0;
    null_move_cutoffs_ = 0;
    futility_prunes_ = 0;
    lmr_searches_ = 0;
    beta_cutoffs_ = 0;
    first_move_cutoffs_ = 0;
    seldepth_ = 0;
    
    // Start search thread
    thread_ = std::thread(&SearchThread::search_loop, this);
}

void SearchThread::stop() {
    stop_ = true;
}

void SearchThread::wait() {
    if (thread_.joinable()) {
        thread_.join();
    }
    searching_ = false;
}

void SearchThread::search_loop() {
    iterative_deepening();
    searching_ = false;
}

bool SearchThread::should_stop() const {
    if (stop_) return true;
    
    // Check node limit
    if (limits_.nodes > 0 && nodes_ >= limits_.nodes) {
        return true;
    }
    
    // Check time limit
    if (limits_.movetime > 0) {
        if (elapsed_time() >= static_cast<uint64_t>(limits_.movetime)) {
            return true;
        }
    } else if (!limits_.infinite && !limits_.ponder) {
        int time_limit = time_for_move();
        if (time_limit > 0 && elapsed_time() >= static_cast<uint64_t>(time_limit)) {
            return true;
        }
    }
    
    return false;
}

uint64_t SearchThread::elapsed_time() const {
    return get_time_ms() - start_time_;
}

int SearchThread::time_for_move() const {
    Color us = root_pos_.side_to_move();
    int time_left = limits_.time[us];
    int increment = limits_.inc[us];
    
    if (time_left <= 0) return 0;
    
    // Simple time management: use 1/20th of remaining time + increment
    int base_time = time_left / 20 + increment;
    
    // Adjust based on moves to go
    if (limits_.movestogo > 0) {
        base_time = time_left / (limits_.movestogo + 1);
    }
    
    return std::max(10, std::min(base_time, time_left / 2));
}

void SearchThread::iterative_deepening() {
    Position pos = root_pos_;
    
    int max_depth = limits_.depth > 0 ? limits_.depth : MAX_PLY;
    int prev_score = 0;
    
    for (int depth = 1; depth <= max_depth && !should_stop(); depth++) {
        // Lazy SMP: vary search parameters slightly per thread
        int alpha = -30000;
        int beta = 30000;
        
        // Use aspiration windows for depth >= 5
        if (depth >= 5 && best_move_.is_valid()) {
            alpha = prev_score - aspiration_delta_;
            beta = prev_score + aspiration_delta_;
        }
        
        int aspiration_delta = aspiration_delta_;
        int fail_count = 0;
        
        // Aspiration window loop
        while (!should_stop()) {
            std::vector<Move> moves;
            MoveGen::generate_legal(pos, moves);
            
            if (moves.empty()) break;
            
            // Order moves
            Move tt_move = MOVE_NONE;
            int score;
            TTFlag flag;
            if (tt_->probe(pos.zobrist_key(), depth - 1, -30000, 30000, 
                          score, tt_move, flag)) {
                // TT move found
            }
            order_moves(pos, moves, tt_move, 0, MOVE_NONE);
            
            Move current_best = moves[0];
            int best_score = -30000;
            std::vector<Move> best_pv;
            
            for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
                Move move = moves[i];
                UndoInfo undo;
                if (!pos.make_move(move, undo)) continue;
                
                std::vector<Move> child_pv;
                int score;
                
                if (i == 0) {
                    // Full window for first move
                    score = -alpha_beta(pos, depth - 1, 1, -beta, -alpha, true, child_pv, move);
                } else {
                    // PVS: null window search
                    score = -alpha_beta(pos, depth - 1, 1, -alpha - 1, -alpha, true, child_pv, move);
                    if (score > alpha && score < beta) {
                        // Re-search with full window
                        score = -alpha_beta(pos, depth - 1, 1, -beta, -alpha, true, child_pv, move);
                    }
                }
                
                pos.unmake_move(move, undo);
                
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
            
            if (should_stop()) break;
            
            // Check aspiration window
            if (best_score <= alpha - aspiration_delta) {
                // Failed low
                alpha = std::max(-30000, alpha - aspiration_delta);
                aspiration_delta *= 2;
                fail_count++;
                if (fail_count > 3) {
                    alpha = -30000;
                    beta = 30000;
                }
                continue;
            } else if (best_score >= beta) {
                // Failed high
                beta = std::min(30000, beta + aspiration_delta);
                aspiration_delta *= 2;
                fail_count++;
                if (fail_count > 3) {
                    alpha = -30000;
                    beta = 30000;
                }
                continue;
            }
            
            // Success
            best_move_ = current_best;
            best_score_ = best_score;
            pv_ = best_pv;
            prev_score = best_score;
            
            // Only main thread outputs info
            if (id_ == 0) {
                std::ostringstream pv_str;
                for (size_t i = 0; i < pv_.size() && i < 10; i++) {
                    if (i > 0) pv_str << " ";
                    Square from = pv_[i].from();
                    Square to = pv_[i].to();
                    pv_str << char('a' + file_of(from)) << char('1' + rank_of(from))
                           << char('a' + file_of(to)) << char('1' + rank_of(to));
                    if (pv_[i].is_promotion()) {
                        switch (pv_[i].promotion()) {
                            case KNIGHT: pv_str << 'n'; break;
                            case BISHOP: pv_str << 'b'; break;
                            case ROOK: pv_str << 'r'; break;
                            case QUEEN: pv_str << 'q'; break;
                            default: break;
                        }
                    }
                }
                
                SearchStatistics temp_stats;
                pool_->get_statistics(temp_stats);
                uint64_t total_nodes = temp_stats.nodes.load();
                uint64_t time_elapsed = elapsed_time();
                uint64_t nps = (time_elapsed > 0) ? (total_nodes * 1000 / time_elapsed) : 0;
                
                std::cout << "info depth " << depth
                          << " seldepth " << seldepth_
                          << " score cp " << best_score_
                          << " nodes " << total_nodes
                          << " nps " << nps
                          << " time " << time_elapsed
                          << " hashfull " << tt_->hashfull()
                          << " pv " << pv_str.str() << std::endl;
                std::cout.flush();
            }
            
            break; // Success, exit aspiration loop
        }
    }
}

int SearchThread::alpha_beta(Position& pos, int depth, int ply, int alpha, int beta,
                              bool null_move_allowed, std::vector<Move>& pv, Move prev_move) {
    nodes_++;
    pv.clear();
    
    if (ply >= MAX_PLY) {
        return evaluator_->evaluate(pos);
    }
    
    if (should_stop()) {
        return alpha;
    }
    
    // Check for draw
    if (pos.is_draw()) return 0;
    
    bool in_check = Attacks::in_check(pos, pos.side_to_move());
    
    // Check extension
    if (in_check) depth++;
    
    // Quiescence at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta, ply);
    }
    
    // Update selective depth
    if (ply > seldepth_) {
        seldepth_ = ply;
    }
    
    // Transposition table probe
    uint64_t key = pos.zobrist_key();
    int tt_score;
    Move tt_move = MOVE_NONE;
    TTFlag tt_flag;
    
    if (tt_->probe(key, depth, alpha, beta, tt_score, tt_move, tt_flag)) {
        if (ply > 0) {
            return tt_score;
        }
    }
    
    // Generate moves
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    if (moves.empty()) {
        if (in_check) {
            return -30000 + ply; // Checkmate
        }
        return 0; // Stalemate
    }
    
    int static_eval = evaluator_->evaluate(pos);
    
    // Null move pruning
    if (null_move_allowed && !in_check && depth >= NULL_MOVE_MIN_DEPTH) {
        // Check if we have non-pawn material
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
                null_move_cutoffs_++;
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
    
    // Razoring
    if (!in_check && depth <= 3) {
        int razor_margin = RAZOR_MARGIN * depth;
        if (static_eval + razor_margin < alpha) {
            int q_score = quiescence(pos, alpha - razor_margin, alpha - razor_margin + 1, ply);
            if (q_score + razor_margin < alpha) {
                return q_score;
            }
        }
    }
    
    // Futility pruning condition
    bool futility_prune = false;
    if (!in_check && depth <= 3 && static_eval + FUTILITY_MARGIN * depth < alpha) {
        futility_prune = true;
    }
    
    // Order moves
    Move counter_move = MOVE_NONE;
    Piece prev_piece = NO_PIECE_PIECE;
    if (prev_move != MOVE_NONE && prev_move.is_valid()) {
        prev_piece = pos.piece_on(prev_move.to());
        if (prev_piece != NO_PIECE_PIECE && prev_piece < 16) {
            counter_move = counter_moves_[prev_piece][prev_move.to()];
        }
    }
    order_moves(pos, moves, tt_move, ply, counter_move);
    
    Move best_move = MOVE_NONE;
    int best_score = -30000;
    TTFlag flag = TT_ALPHA;
    std::vector<Move> best_pv;
    
    int legal_moves = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        legal_moves++;
        
        // Futility pruning
        if (futility_prune && !move.is_capture() && !move.is_promotion() &&
            !Attacks::in_check(pos, pos.side_to_move())) {
            pos.unmake_move(move, undo);
            futility_prunes_++;
            continue;
        }
        
        int score;
        std::vector<Move> child_pv;
        
        // Late Move Reduction
        bool do_lmr = legal_moves >= LMR_MIN_MOVES && depth >= LMR_MIN_DEPTH &&
                      !in_check && !move.is_capture() && !move.is_promotion() &&
                      !Attacks::in_check(pos, pos.side_to_move());
        
        if (do_lmr) {
            int reduction = 1;
            if (legal_moves > 6 && depth > 6) reduction = 2;
            
            score = -alpha_beta(pos, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, true, child_pv, move);
            lmr_searches_++;
            
            if (score > alpha) {
                score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
            }
        } else {
            if (legal_moves == 1) {
                score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
            } else {
                score = -alpha_beta(pos, depth - 1, ply + 1, -alpha - 1, -alpha, true, child_pv, move);
                if (score > alpha && score < beta) {
                    score = -alpha_beta(pos, depth - 1, ply + 1, -beta, -alpha, true, child_pv, move);
                }
            }
        }
        
        pos.unmake_move(move, undo);
        
        if (should_stop()) return alpha;
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
            best_pv.clear();
            best_pv.push_back(move);
            best_pv.insert(best_pv.end(), child_pv.begin(), child_pv.end());
        }
        
        if (score >= beta) {
            // Beta cutoff
            beta_cutoffs_++;
            if (legal_moves == 1) {
                first_move_cutoffs_++;
            }
            
            if (!move.is_capture() && ply < MAX_PLY) {
                // Update killer moves
                if (killers_[ply][0].data != move.data) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = move;
                }
                
                // Update history
                history_[pos.side_to_move()][move.from()][move.to()] += depth * depth;
                if (history_[pos.side_to_move()][move.from()][move.to()] > 10000) {
                    // Age history
                    for (int c = 0; c < 2; c++) {
                        for (int f = 0; f < 64; f++) {
                            for (int t = 0; t < 64; t++) {
                                history_[c][f][t] /= 2;
                            }
                        }
                    }
                }
            }
            
            tt_->store(key, depth, beta, move, TT_BETA);
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;
            // Update counter move heuristic when we find a new best move
            if (best_move != MOVE_NONE && best_move.is_valid() && prev_move != MOVE_NONE && prev_move.is_valid() &&
                !best_move.is_capture() && prev_piece != NO_PIECE_PIECE && prev_piece < 16) {
                counter_moves_[prev_piece][prev_move.to()] = best_move;
            }
        }
    }
    
    pv = best_pv;
    tt_->store(key, depth, best_score, best_move, flag);
    return best_score;
}

int SearchThread::quiescence(Position& pos, int alpha, int beta, int ply) {
    qnodes_++;
    
    if (ply >= MAX_PLY) {
        return evaluator_->evaluate(pos);
    }
    
    if (should_stop()) {
        return alpha;
    }
    
    bool in_check = Attacks::in_check(pos, pos.side_to_move());
    
    // Stand pat - but not if we're in check (must search all moves)
    int stand_pat = evaluator_->evaluate(pos);
    if (!in_check) {
        if (stand_pat >= beta) {
            return beta;
        }
        if (stand_pat > alpha) {
            alpha = stand_pat;
        }
    }
    
    // Generate captures (and all moves if in check)
    std::vector<Move> moves;
    if (in_check) {
        // If in check, must search all legal moves
        MoveGen::generate_legal(pos, moves);
        if (moves.empty()) {
            // Checkmate
            return -30000 + ply;
        }
    } else {
        MoveGen::generate_captures(pos, moves);
        
        if (moves.empty()) {
            return stand_pat;
        }
    }
    
    order_moves(pos, moves, MOVE_NONE, ply, MOVE_NONE);
    
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
        
        int score = -quiescence(pos, -beta, -alpha, ply + 1);
        pos.unmake_move(move, undo);
        
        if (should_stop()) return alpha;
        
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

void SearchThread::order_moves(const Position& pos, std::vector<Move>& moves,
                                Move tt_move, int ply, Move counter_move) {
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        int score_a = 0, score_b = 0;
        
        // TT move
        if (a.data == tt_move.data) return true;
        if (b.data == tt_move.data) return false;
        
        // Promotions (especially queen promotions) - very high priority
        if (a.is_promotion()) {
            score_a += 11000000;
            if (a.promotion() == QUEEN) score_a += 1000000;
        }
        if (b.is_promotion()) {
            score_b += 11000000;
            if (b.promotion() == QUEEN) score_b += 1000000;
        }
        
        // Captures
        if (a.is_capture()) {
            int see_a = see(pos, a);
            score_a += (see_a > 0) ? 10000000 + see_a : 5000000 + see_a;
        }
        if (b.is_capture()) {
            int see_b = see(pos, b);
            score_b += (see_b > 0) ? 10000000 + see_b : 5000000 + see_b;
        }
        
        // Killers
        if (!a.is_capture() && ply < MAX_PLY) {
            if (a.data == killers_[ply][0].data) score_a += 8000000;
            else if (a.data == killers_[ply][1].data) score_a += 7000000;
        }
        if (!b.is_capture() && ply < MAX_PLY) {
            if (b.data == killers_[ply][0].data) score_b += 8000000;
            else if (b.data == killers_[ply][1].data) score_b += 7000000;
        }
        
        // Counter move heuristic
        if (!a.is_capture() && a.data == counter_move.data) score_a += 6000000;
        if (!b.is_capture() && b.data == counter_move.data) score_b += 6000000;
        
        // History
        if (!a.is_capture()) {
            score_a += history_[pos.side_to_move()][a.from()][a.to()];
        }
        if (!b.is_capture()) {
            score_b += history_[pos.side_to_move()][b.from()][b.to()];
        }
        
        return score_a > score_b;
    });
}

int SearchThread::see(const Position& pos, Move move) const {
    Square to = move.to();
    Square from = move.from();
    
    Piece captured = pos.piece_on(to);
    if (captured == NO_PIECE_PIECE && !move.is_en_passant()) {
        return 0;
    }
    
    Piece attacker = pos.piece_on(from);
    int gain = SEE_VALUES[type_of(captured)];
    
    if (move.is_promotion()) {
        gain -= SEE_VALUES[PAWN];
        gain += SEE_VALUES[move.promotion()];
    }
    
    int attacker_value = SEE_VALUES[type_of(attacker)];
    if (move.is_promotion()) {
        attacker_value = SEE_VALUES[move.promotion()];
    }
    
    if (gain >= attacker_value) {
        return gain;
    }
    
    return gain - attacker_value;
}

} // namespace chess
