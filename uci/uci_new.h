#pragma once

#include "../board/position.h"
#include "../search/thread_pool.h"
#include "../search/tt.h"
#include "../eval/evaluator.h"
#include "../bench/bench.h"
#include <string>
#include <sstream>
#include <memory>
#include <thread>
#include <atomic>

namespace chess {

// Modern UCI interface with multi-threading support
class UCINew {
public:
    UCINew();
    ~UCINew();
    
    void loop();
    
private:
    void handle_uci();
    void handle_isready();
    void handle_ucinewgame();
    void handle_position(const std::string& line);
    void handle_go(const std::string& line);
    void handle_stop();
    void handle_ponderhit();
    void handle_setoption(const std::string& line);
    void handle_bench(const std::string& line);
    void handle_perft(const std::string& line);
    void handle_d();
    void handle_eval();
    void handle_compiler();
    
    void print_options();
    void switch_evaluator(bool use_nnue);
    
    Move parse_move(const std::string& move_str);
    std::string move_to_string(Move move);
    
    Position pos_;
    std::unique_ptr<TranspositionTable> tt_;
    std::unique_ptr<ThreadPool> pool_;
    Evaluator* evaluator_;
    bool use_nnue_;
    std::string nnue_file_;
    
    // UCI options
    int threads_;
    int hash_size_mb_;
    int multi_pv_;
    bool ponder_;
    
    // Search thread for non-blocking search
    std::unique_ptr<std::thread> search_thread_;
    std::atomic<bool> searching_;
};

} // namespace chess
