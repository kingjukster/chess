#pragma once

#include "../board/position.h"
#include "../search/thread_pool.h"
#include <string>
#include <vector>

namespace chess {

// Benchmark utility for standardized performance testing
class Benchmark {
public:
    struct BenchResult {
        uint64_t total_nodes;
        uint64_t total_qnodes;
        uint64_t total_time_ms;
        uint64_t nps;
        int positions_tested;
        std::vector<std::string> position_fens;
        std::vector<uint64_t> position_nodes;
        std::vector<int> position_depths;
    };
    
    // Run standard benchmark suite
    static BenchResult run_bench(ThreadPool* pool, int depth = 13);
    
    // Run benchmark on custom positions
    static BenchResult run_custom_bench(ThreadPool* pool, 
                                        const std::vector<std::string>& fens,
                                        int depth = 13);
    
    // Get standard benchmark positions (similar to Stockfish)
    static std::vector<std::string> get_bench_positions();
    
    // Print benchmark results
    static void print_results(const BenchResult& result);
};

} // namespace chess
