#include "search.h"
#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include "../board/types.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace chess {

Search::Search(Evaluator* eval) : evaluator(eval), time_limit_ms(0), stop_search(false) {
    // Evaluator will be initialized when position is set
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

TTEntry* Search::probe_tt(uint64_t key, int depth, int alpha, int beta) {
    auto it = tt.find(key);
    if (it == tt.end()) return nullptr;
    
    TTEntry& entry = it->second;
    if (entry.key != key) return nullptr;
    if (entry.depth < depth) return nullptr;
    
    // Check if entry is usable
    if (entry.flag == 0) { // Exact
        return &entry;
    } else if (entry.flag == 1 && entry.score <= alpha) { // Alpha
        return &entry;
    } else if (entry.flag == 2 && entry.score >= beta) { // Beta
        return &entry;
    }
    
    return nullptr;
}

void Search::store_tt(uint64_t key, int depth, int score, Move move, int flag) {
    TTEntry entry;
    entry.key = key;
    entry.depth = depth;
    entry.score = score;
    entry.best_move = move;
    entry.flag = flag;
    entry.age = 0;
    
    tt[key] = entry;
    
    // Limit TT size
    if (tt.size() > TT_SIZE) {
        // Simple: remove oldest entries (in production, use better replacement)
        auto it = tt.begin();
        std::advance(it, tt.size() / 2);
        tt.erase(tt.begin(), it);
    }
}

void Search::order_moves(const Position& pos, std::vector<Move>& moves, Move tt_move) {
    // MVV-LVA ordering
    std::sort(moves.begin(), moves.end(), [&pos, tt_move](const Move& a, const Move& b) {
        // TT move first
        if (a.data == tt_move.data) return true;
        if (b.data == tt_move.data) return false;
        
        // Captures first
        if (a.is_capture() && !b.is_capture()) return true;
        if (!a.is_capture() && b.is_capture()) return false;
        
        // MVV-LVA for captures
        if (a.is_capture() && b.is_capture()) {
            Piece captured_a = pos.piece_on(a.to());
            Piece captured_b = pos.piece_on(b.to());
            int value_a = (captured_a == NO_PIECE_PIECE) ? 0 : (1 << type_of(captured_a));
            int value_b = (captured_b == NO_PIECE_PIECE) ? 0 : (1 << type_of(captured_b));
            if (value_a != value_b) return value_a > value_b;
        }
        
        // Promotions
        if (a.is_promotion() && !b.is_promotion()) return true;
        if (!a.is_promotion() && b.is_promotion()) return false;
        
        return false;
    });
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
    
    // Stand pat
    int stand_pat = evaluator->evaluate(pos);
    if (stand_pat >= beta) {
        qdepth--;
        return beta;
    }
    if (stand_pat > alpha) alpha = stand_pat;
    
    // Generate captures
    std::vector<Move> moves;
    MoveGen::generate_captures(pos, moves);
    
    if (moves.empty()) {
        qdepth--;
        return stand_pat;
    }
    
    order_moves(pos, moves, MOVE_NONE);
    
    for (Move move : moves) {
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

int Search::alpha_beta(Position& pos, int depth, int alpha, int beta, bool null_move_allowed) {
    stats.nodes++;
    
    // Safety check - prevent infinite recursion
    if (stats.nodes > 1000000) {
        stop_search = true;
        return evaluator->evaluate(pos);
    }
    
    if (time_over()) {
        stop_search = true;
        return alpha;
    }
    
    // Check for draw
    if (pos.is_draw()) return 0;
    
    // Check for checkmate/stalemate
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    if (moves.empty()) {
        if (Attacks::in_check(pos, pos.side_to_move())) {
            return -30000 + (1000 - stats.depth); // Checkmate
        }
        return 0; // Stalemate
    }
    
    // Transposition table probe
    uint64_t key = pos.zobrist_key();
    TTEntry* tt_entry = probe_tt(key, depth, alpha, beta);
    if (tt_entry) {
        return tt_entry->score;
    }
    
    // Quiescence at leaf nodes
    if (depth <= 0) {
        return quiescence(pos, alpha, beta);
    }
    
    // Null move pruning (simplified - skip for now, requires Position modification)
    // TODO: Implement proper null move with Position support
    
    Move best_move = MOVE_NONE;
    int best_score = -30000;
    int flag = 1; // Alpha flag
    
    Move tt_move = tt_entry ? tt_entry->best_move : MOVE_NONE;
    order_moves(pos, moves, tt_move);
    
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        
        int score;
        
        // Late move reduction
        bool lmr = (i >= LMR_MIN_MOVES && depth >= LMR_MIN_DEPTH && !move.is_capture() && !Attacks::in_check(pos, pos.side_to_move()));
        
        if (lmr) {
            score = -alpha_beta(pos, depth - 2, -beta, -alpha, true);
            if (score > alpha) {
                score = -alpha_beta(pos, depth - 1, -beta, -alpha, true);
            }
        } else {
            score = -alpha_beta(pos, depth - 1, -beta, -alpha, true);
        }
        
        pos.unmake_move(move, undo);
        
        if (stop_search) return alpha;
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        
        if (score >= beta) {
            // Beta cutoff
            store_tt(key, depth, beta, move, 2); // Beta flag
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
            flag = 0; // Exact flag
        }
    }
    
    store_tt(key, depth, best_score, best_move, flag);
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
    
    int alpha = -30000;
    int beta = 30000;
    Move best_move = moves[0];
    
    for (Move move : moves) {
        UndoInfo undo;
        if (!pos.make_move(move, undo)) continue;
        
        int score = -alpha_beta(pos, depth - 1, -beta, -alpha, true);
        pos.unmake_move(move, undo);
        
        if (stop_search) break;
        
        if (score > alpha) {
            alpha = score;
            best_move = move;
            stats.best_score = score;
        }
    }
    
    stats.best_move = best_move;
    return best_move;
}

Move Search::iterative_deepening(Position& pos, int max_depth, int time_limit) {
    stats = SearchStats();
    stop_search = false;
    time_limit_ms = time_limit;
    start_time = get_time_ms();
    
    Move best_move = MOVE_NONE;
    
    try {
        for (int depth = 1; depth <= max_depth && !stop_search; depth++) {
            stats.depth = depth;
            stats.nodes = 0;  // Reset node count for this depth
            
            std::vector<Move> moves;
            MoveGen::generate_legal(pos, moves);
            if (moves.empty()) break;
            
            int alpha = -30000;
            int beta = 30000;
            Move current_best = moves[0];
            
            for (size_t i = 0; i < moves.size() && !stop_search; i++) {
                Move move = moves[i];
                UndoInfo undo;
                if (!pos.make_move(move, undo)) continue;
                
                int score = -alpha_beta(pos, depth - 1, -beta, -alpha, true);
                pos.unmake_move(move, undo);
                
                if (stop_search) break;
                
                if (score > alpha) {
                    alpha = score;
                    current_best = move;
                    stats.best_score = score;
                }
            }
            
            if (!stop_search) {
                best_move = current_best;
                stats.best_move = best_move;
                
                // Output info for this depth (UCI format)
                std::string move_str;
                Square from = current_best.from();
                Square to = current_best.to();
                move_str += char('a' + file_of(from));
                move_str += char('1' + rank_of(from));
                move_str += char('a' + file_of(to));
                move_str += char('1' + rank_of(to));
                if (current_best.is_promotion()) {
                    switch (current_best.promotion()) {
                        case KNIGHT: move_str += 'n'; break;
                        case BISHOP: move_str += 'b'; break;
                        case ROOK: move_str += 'r'; break;
                        case QUEEN: move_str += 'q'; break;
                        default: break;
                    }
                }
                
                std::cout << "info depth " << depth << " nodes " << stats.nodes 
                          << " score cp " << alpha << " pv " << move_str << std::endl;
                std::cout.flush();
            }
        }
    } catch (...) {
        // Catch any exceptions and return best move found so far
        std::cerr << "Exception in search!" << std::endl;
    }
    
    return best_move;
}

void Search::clear_tt() {
    tt.clear();
}

} // namespace chess

