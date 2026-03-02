#include "bench.h"
#include "../eval/evaluator.h"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace chess {

std::vector<std::string> Benchmark::get_bench_positions() {
    // Standard benchmark positions covering various game phases
    return {
        // Starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        
        // Middlegame positions
        "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        
        // Endgame positions
        "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1",
        "rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
        "8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - - 1 67",
        
        // Tactical positions
        "r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQK2R w KQ - 0 7",
        "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 7",
        
        // Complex positions
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
        
        // King safety positions
        "r1bq1rk1/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - - 1 17",
        "5rk1/1ppb3p/3b4/2Pp4/3P2q1/2P1R1p1/P4r2/1R4QK b - - 0 1",
        
        // Pawn structure positions
        "rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6",
        "r1bqkbnr/pppp1p1p/2n3p1/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4"
    };
}

Benchmark::BenchResult Benchmark::run_bench(ThreadPool* pool, int depth) {
    return run_custom_bench(pool, get_bench_positions(), depth);
}

Benchmark::BenchResult Benchmark::run_custom_bench(ThreadPool* pool,
                                                    const std::vector<std::string>& fens,
                                                    int depth) {
    BenchResult result;
    result.total_nodes = 0;
    result.total_qnodes = 0;
    result.total_time_ms = 0;
    result.positions_tested = 0;
    
    std::cout << "Running benchmark with " << fens.size() << " positions at depth " << depth << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < fens.size(); i++) {
        const std::string& fen = fens[i];
        
        std::cout << "Position " << (i + 1) << "/" << fens.size() << ": ";
        std::cout.flush();
        
        Position pos;
        try {
            pos.from_fen(fen);
        } catch (...) {
            std::cout << "Invalid FEN, skipping" << std::endl;
            continue;
        }
        
        // Set up search limits
        SearchLimits limits;
        limits.depth = depth;
        
        // Run search
        auto pos_start = std::chrono::steady_clock::now();
        pool->start_search(pos, limits);
        pool->wait_for_search();
        auto pos_end = std::chrono::steady_clock::now();
        
        // Get statistics
        SearchStatistics stats;
        pool->get_statistics(stats);
        uint64_t pos_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            pos_end - pos_start).count();
        
        result.position_fens.push_back(fen);
        result.position_nodes.push_back(stats.nodes);
        result.position_depths.push_back(depth);
        result.total_nodes += stats.nodes;
        result.total_qnodes += stats.qnodes;
        result.total_time_ms += pos_time;
        result.positions_tested++;
        
        uint64_t pos_nps = pos_time > 0 ? (stats.nodes * 1000 / pos_time) : 0;
        std::cout << stats.nodes << " nodes, " << pos_time << " ms, "
                  << pos_nps << " nps" << std::endl;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    result.nps = result.total_time_ms > 0 ? 
                 (result.total_nodes * 1000 / result.total_time_ms) : 0;
    
    return result;
}

void Benchmark::print_results(const BenchResult& result) {
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Benchmark Results:" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "Positions tested: " << result.positions_tested << std::endl;
    std::cout << "Total nodes:      " << result.total_nodes << std::endl;
    std::cout << "Total qnodes:     " << result.total_qnodes << std::endl;
    std::cout << "Total time:       " << result.total_time_ms << " ms" << std::endl;
    std::cout << "Nodes per second: " << result.nps << " nps" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

} // namespace chess
