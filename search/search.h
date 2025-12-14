#pragma once

#include "../board/types.h"
#include "../board/position.h"
#include "../eval/evaluator.h"
#include <vector>
#include <unordered_map>

namespace chess {

// Transposition table entry
struct TTEntry {
    uint64_t key;
    int depth;
    int score;
    Move best_move;
    uint8_t flag; // 0=exact, 1=alpha, 2=beta
    uint8_t age;
    
    TTEntry() : key(0), depth(0), score(0), best_move(MOVE_NONE), flag(0), age(0) {}
};

// Search statistics
struct SearchStats {
    uint64_t nodes;
    uint64_t qnodes;
    int depth;
    Move best_move;
    int best_score;
    
    SearchStats() : nodes(0), qnodes(0), depth(0), best_move(MOVE_NONE), best_score(0) {}
};

class Search {
public:
    Search(Evaluator* eval);
    ~Search();
    
    // Main search function
    Move search(Position& pos, int depth, int time_limit_ms = 0);
    
    // Iterative deepening
    Move iterative_deepening(Position& pos, int max_depth, int time_limit_ms = 0);
    
    // Get search statistics
    const SearchStats& get_stats() const { return stats; }
    
    // Clear transposition table
    void clear_tt();
    
    // Set time management
    void set_time_limit(int ms) { time_limit_ms = ms; }

private:
    // Alpha-beta search
    int alpha_beta(Position& pos, int depth, int alpha, int beta, bool null_move_allowed);
    
    // Quiescence search
    int quiescence(Position& pos, int alpha, int beta);
    
    // Move ordering
    void order_moves(const Position& pos, std::vector<Move>& moves, Move tt_move);
    
    // Transposition table
    TTEntry* probe_tt(uint64_t key, int depth, int alpha, int beta);
    void store_tt(uint64_t key, int depth, int score, Move move, int flag);
    
    // Time management
    bool time_over() const;
    
    Evaluator* evaluator;
    SearchStats stats;
    std::unordered_map<uint64_t, TTEntry> tt;
    int time_limit_ms;
    uint64_t start_time;
    bool stop_search;
    
    // Search parameters
    static const int TT_SIZE = 1024 * 1024; // 1M entries
    static const int NULL_MOVE_R = 2;
    static const int LMR_MIN_DEPTH = 3;
    static const int LMR_MIN_MOVES = 3;
};

} // namespace chess

