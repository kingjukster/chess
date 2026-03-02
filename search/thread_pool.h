#pragma once

#include "../board/position.h"
#include "../board/types.h"
#include "tt.h"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace chess {

class Evaluator;
class SearchThread;

// Search limits
struct SearchLimits {
    int depth;              // Max depth (0 = unlimited)
    uint64_t nodes;         // Max nodes (0 = unlimited)
    int mate;               // Mate in N moves (0 = disabled)
    int movetime;           // Exact time in ms (0 = disabled)
    int time[COLOR_NB];     // Time remaining for each side
    int inc[COLOR_NB];      // Increment per move
    int movestogo;          // Moves to next time control (0 = sudden death)
    bool infinite;          // Search until "stop" command
    bool ponder;            // Pondering mode
    
    SearchLimits() : depth(0), nodes(0), mate(0), movetime(0), 
                     movestogo(0), infinite(false), ponder(false) {
        time[WHITE] = time[BLACK] = 0;
        inc[WHITE] = inc[BLACK] = 0;
    }
};

// Search statistics (aggregated across all threads)
struct SearchStatistics {
    std::atomic<uint64_t> nodes;
    std::atomic<uint64_t> qnodes;
    std::atomic<uint64_t> tb_hits;
    std::atomic<uint64_t> null_move_cutoffs;
    std::atomic<uint64_t> futility_prunes;
    std::atomic<uint64_t> lmr_searches;
    std::atomic<uint64_t> beta_cutoffs;
    std::atomic<uint64_t> first_move_cutoffs;
    
    int seldepth;
    int depth;
    Move best_move;
    int best_score;
    std::vector<Move> pv;
    
    SearchStatistics() : nodes(0), qnodes(0), tb_hits(0), null_move_cutoffs(0),
                         futility_prunes(0), lmr_searches(0), beta_cutoffs(0),
                         first_move_cutoffs(0), seldepth(0), depth(0),
                         best_move(MOVE_NONE), best_score(0) {}
    
    void reset() {
        nodes = 0;
        qnodes = 0;
        tb_hits = 0;
        null_move_cutoffs = 0;
        futility_prunes = 0;
        lmr_searches = 0;
        beta_cutoffs = 0;
        first_move_cutoffs = 0;
        seldepth = 0;
        depth = 0;
        best_move = MOVE_NONE;
        best_score = 0;
        pv.clear();
    }
};

// Thread pool for Lazy SMP parallel search
class ThreadPool {
public:
    ThreadPool(Evaluator* eval, TranspositionTable* tt);
    ~ThreadPool();
    
    // Set number of threads
    void set_threads(int count);
    
    // Start search on all threads
    void start_search(const Position& pos, const SearchLimits& limits);
    
    // Stop all threads
    void stop_search();
    
    // Wait for search to complete
    void wait_for_search();
    
    // Check if search is running
    bool is_searching() const { return searching_; }
    
    // Get best move found so far
    Move best_move() const;
    
    // Get search statistics
    void get_statistics(SearchStatistics& stats) const;
    
    // Get main thread for UCI communication
    SearchThread* main_thread() const { return threads_[0].get(); }
    
    // Pondering control
    void start_pondering(const Position& pos, Move ponder_move);
    void ponderhit();
    
private:
    std::vector<std::unique_ptr<SearchThread>> threads_;
    Evaluator* evaluator_;
    TranspositionTable* tt_;
    std::atomic<bool> searching_;
    std::atomic<bool> stop_;
    int num_threads_;
};

// Individual search thread (Lazy SMP)
class SearchThread {
public:
    SearchThread(int id, Evaluator* eval, TranspositionTable* tt, ThreadPool* pool);
    ~SearchThread();
    
    // Start searching
    void start(const Position& pos, const SearchLimits& limits);
    
    // Stop searching
    void stop();
    
    // Wait for thread to finish
    void wait();
    
    // Check if thread is searching
    bool is_searching() const { return searching_; }
    
    // Get best move found
    Move best_move() const { return best_move_; }
    
    // Get best score
    int best_score() const { return best_score_; }
    
    // Get principal variation
    const std::vector<Move>& pv() const { return pv_; }
    
    // Get thread ID
    int id() const { return id_; }
    
    // Get statistics
    uint64_t nodes() const { return nodes_; }
    uint64_t qnodes() const { return qnodes_; }
    int seldepth() const { return seldepth_; }
    
    // Get null move cutoffs
    uint64_t null_move_cutoffs() const { return null_move_cutoffs_; }
    uint64_t futility_prunes() const { return futility_prunes_; }
    uint64_t lmr_searches() const { return lmr_searches_; }
    uint64_t beta_cutoffs() const { return beta_cutoffs_; }
    uint64_t first_move_cutoffs() const { return first_move_cutoffs_; }

private:
    // Main search loop
    void search_loop();
    
    // Iterative deepening
    void iterative_deepening();
    
    // Alpha-beta search
    int alpha_beta(Position& pos, int depth, int ply, int alpha, int beta, 
                   bool null_move_allowed, std::vector<Move>& pv, Move prev_move = MOVE_NONE);
    
    // Quiescence search
    int quiescence(Position& pos, int alpha, int beta, int ply);
    
    // Move ordering
    void order_moves(const Position& pos, std::vector<Move>& moves, 
                     Move tt_move, int ply, Move counter_move = MOVE_NONE);
    
    // Static Exchange Evaluation
    int see(const Position& pos, Move move) const;
    
    // Time management
    bool should_stop() const;
    uint64_t elapsed_time() const;
    int time_for_move() const;
    
    int id_;
    Evaluator* evaluator_;
    TranspositionTable* tt_;
    ThreadPool* pool_;
    
    std::thread thread_;
    std::atomic<bool> searching_;
    std::atomic<bool> stop_;
    
    Position root_pos_;
    SearchLimits limits_;
    uint64_t start_time_;
    
    // Search state
    Move best_move_;
    int best_score_;
    std::vector<Move> pv_;
    int seldepth_;
    
    // Statistics
    uint64_t nodes_;
    uint64_t qnodes_;
    uint64_t null_move_cutoffs_;
    uint64_t futility_prunes_;
    uint64_t lmr_searches_;
    uint64_t beta_cutoffs_;
    uint64_t first_move_cutoffs_;
    
    // History heuristic: [side_to_move][from][to]
    int history_[2][64][64];
    
    // Killer moves: [ply][slot]
    Move killers_[256][2];
    
    // Counter moves: [piece][to_square]
    Move counter_moves_[16][64];
    
    // Aspiration window for Lazy SMP variation
    int aspiration_delta_;
};

} // namespace chess
