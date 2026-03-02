#pragma once

#include "../board/types.h"
#include "../board/position.h"
#include "../eval/evaluator.h"
#include "../tablebase/syzygy.h"
#include "tt.h"
#include <vector>
#include <array>
#include <memory>

namespace chess {

// Search statistics
struct SearchStats {
    uint64_t nodes;
    uint64_t qnodes;
    uint64_t null_move_cutoffs;
    uint64_t futility_prunes;
    uint64_t lmr_searches;
    int depth;
    Move best_move;
    int best_score;
    std::vector<Move> pv_lines[256]; // Multi-PV support
    
    SearchStats() : nodes(0), qnodes(0), null_move_cutoffs(0), futility_prunes(0), 
                    lmr_searches(0), depth(0), best_move(MOVE_NONE), best_score(0) {}
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
    
    // UCI options
    void set_multi_pv(int count) { multi_pv = count; }
    void set_null_move_enabled(bool enabled) { use_null_move = enabled; }
    void set_lmr_enabled(bool enabled) { use_lmr = enabled; }
    void set_futility_enabled(bool enabled) { use_futility = enabled; }
    void set_tablebase(struct SyzygyTablebase* tb) { tablebase = tb; }
    void set_hash_size(size_t mb);
    TranspositionTable* get_tt() { return tt_.get(); }

private:
    // Alpha-beta search with PV tracking
    int alpha_beta(Position& pos, int depth, int ply, int alpha, int beta, 
                   bool null_move_allowed, std::vector<Move>& pv, Move prev_move = MOVE_NONE);
    
    // Quiescence search
    int quiescence(Position& pos, int alpha, int beta);
    
    // Move ordering
    void order_moves(const Position& pos, std::vector<Move>& moves, Move tt_move, 
                     int ply, Move counter_move);
    
    // Static Exchange Evaluation
    int see(const Position& pos, Move move) const;
    int see_recursive(Position& pos, Square to_sq, int depth) const;
    
    // Transposition table (bucket hash)
    struct TTProbeResult { bool found; bool usable; int score; Move move; };
    TTProbeResult probe_tt(uint64_t key, int depth, int alpha, int beta, int ply);
    void store_tt(uint64_t key, int depth, int score, Move move, int flag, int ply);
    
    // Time management
    bool time_over() const;
    
    Evaluator* evaluator;
    struct SyzygyTablebase* tablebase;
    SearchStats stats;
    std::unique_ptr<TranspositionTable> tt_;
    int time_limit_ms;
    uint64_t start_time;
    bool stop_search;
    
    // History heuristic: [side_to_move][from][to]
    int history[2][64][64];
    
    // Killer moves: [ply][slot]
    Move killers[256][2];
    
    // Counter moves: [piece][to_square]
    Move counter_moves[16][64];
    
    // UCI options
    int multi_pv;
    bool use_null_move;
    bool use_lmr;
    bool use_futility;
    
    // Search parameters (tunable via UCI)
    static const int NULL_MOVE_R = 3;  // Increased from 2 to 3 for more aggressive pruning
    static const int NULL_MOVE_MIN_DEPTH = 3;
    static const int LMR_MIN_DEPTH = 3;
    static const int LMR_MIN_MOVES = 4;  // Increased from 3 to 4 - reduce later moves more
    static const int FUTILITY_MARGIN = 120;  // Increased from 100 to 120 for better pruning
    static const int RAZOR_MARGIN = 350;  // Increased from 300 to 350
    static const int ASPIRATION_WINDOW = 40;  // Decreased from 50 to 40 for tighter window
    static const int MAX_PLY = 256;
    
    // Piece values for SEE
    static const int SEE_VALUES[7];
};

} // namespace chess

