#include "epd_runner.h"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace chess {

EpdRunner::EpdRunner(Search* search) : search(search) {}

EpdTestResult EpdRunner::run_suite(const std::string& filename, int depth, int time_limit_ms) {
    EpdTestResult result;
    
    std::cout << "Loading EPD test suite: " << filename << std::endl;
    std::vector<EpdEntry> entries = EpdParser::load_file(filename);
    
    if (entries.empty()) {
        std::cerr << "Error: No positions loaded from " << filename << std::endl;
        return result;
    }
    
    result.total_positions = entries.size();
    std::cout << "Loaded " << entries.size() << " positions" << std::endl;
    std::cout << "Testing with depth=" << depth << ", time_limit=" << time_limit_ms << "ms" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    auto suite_start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < entries.size(); i++) {
        const EpdEntry& entry = entries[i];
        
        std::cout << "\rPosition " << (i + 1) << "/" << entries.size() << " ";
        if (!entry.id.empty()) {
            std::cout << "[" << entry.id << "] ";
        }
        std::cout.flush();
        
        bool passed = test_position(entry, depth, time_limit_ms);
        
        if (passed) {
            result.passed++;
            std::cout << "✓ PASS";
        } else {
            result.failed++;
            std::cout << "✗ FAIL";
            if (!entry.id.empty()) {
                result.failed_ids.push_back(entry.id);
            }
        }
        
        if (!entry.description.empty()) {
            std::cout << " - " << entry.description;
        }
        std::cout << std::endl;
    }
    
    auto suite_end = std::chrono::high_resolution_clock::now();
    result.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(suite_end - suite_start).count();
    
    result.pass_rate = result.total_positions > 0 ? 
        (100.0 * result.passed / result.total_positions) : 0.0;
    
    return result;
}

bool EpdRunner::test_position(const EpdEntry& entry, int depth, int time_limit_ms) {
    Position pos(entry.fen);
    
    // Initialize evaluator
    if (pos.evaluator) {
        pos.evaluator->initialize(pos);
    }
    
    // Clear search state
    search->clear_tt();
    
    // Run search
    Move best_move = search->search(pos, depth, time_limit_ms);
    
    // Check if best move matches expected moves
    bool correct = false;
    
    if (!entry.best_moves.empty()) {
        correct = is_correct_move(best_move, entry.best_moves, pos);
    }
    
    // Also check avoid moves (if present, best move should NOT be in avoid list)
    if (!entry.avoid_moves.empty()) {
        bool avoided = !is_correct_move(best_move, entry.avoid_moves, pos);
        if (!entry.best_moves.empty()) {
            correct = correct && avoided;
        } else {
            correct = avoided;
        }
    }
    
    return correct;
}

bool EpdRunner::is_correct_move(Move move, const std::vector<std::string>& expected_moves, const Position& pos) {
    std::string move_str = move_to_string(move);
    
    for (const std::string& expected : expected_moves) {
        // Direct match
        if (move_str == expected) {
            return true;
        }
        
        // Try SAN notation match (simplified - just check if move squares match)
        // This is a basic implementation; full SAN parsing would be more complex
        if (expected.length() >= 2) {
            // Check if it's a coordinate move (e.g., e2e4)
            if (expected.length() >= 4 && expected[0] >= 'a' && expected[0] <= 'h' &&
                expected[1] >= '1' && expected[1] <= '8') {
                if (move_str == expected) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

std::string EpdRunner::move_to_string(Move move) {
    if (!move.is_valid()) return "0000";
    
    Square from = move.from();
    Square to = move.to();
    
    std::string result;
    result += char('a' + file_of(from));
    result += char('1' + rank_of(from));
    result += char('a' + file_of(to));
    result += char('1' + rank_of(to));
    
    if (move.is_promotion()) {
        switch (move.promotion()) {
            case KNIGHT: result += 'n'; break;
            case BISHOP: result += 'b'; break;
            case ROOK: result += 'r'; break;
            case QUEEN: result += 'q'; break;
            default: break;
        }
    }
    
    return result;
}

void EpdRunner::print_results(const EpdTestResult& result) {
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "EPD TEST RESULTS" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Total positions: " << result.total_positions << std::endl;
    std::cout << "Passed: " << result.passed << std::endl;
    std::cout << "Failed: " << result.failed << std::endl;
    std::cout << "Pass rate: " << std::fixed << std::setprecision(1) 
              << result.pass_rate << "%" << std::endl;
    std::cout << "Total time: " << result.total_time_ms << " ms" << std::endl;
    
    if (!result.failed_ids.empty()) {
        std::cout << "\nFailed positions:" << std::endl;
        for (const std::string& id : result.failed_ids) {
            std::cout << "  - " << id << std::endl;
        }
    }
    
    std::cout << std::string(80, '=') << std::endl;
}

int EpdRunner::estimate_elo(double pass_rate, const std::string& suite_name) {
    // Rough ELO estimates based on test suite performance
    // These are very approximate and depend on the specific test suite
    
    if (suite_name.find("WAC") != std::string::npos) {
        // Win At Chess - tactical test suite
        // ~300 positions, tactical puzzles
        if (pass_rate >= 95) return 2400;
        if (pass_rate >= 90) return 2200;
        if (pass_rate >= 80) return 2000;
        if (pass_rate >= 70) return 1800;
        if (pass_rate >= 60) return 1600;
        return 1400;
    } else if (suite_name.find("Arasan") != std::string::npos) {
        // Arasan test suite - mixed positions
        if (pass_rate >= 90) return 2500;
        if (pass_rate >= 80) return 2300;
        if (pass_rate >= 70) return 2100;
        if (pass_rate >= 60) return 1900;
        return 1700;
    } else if (suite_name.find("STS") != std::string::npos) {
        // Strategic Test Suite - positional understanding
        if (pass_rate >= 85) return 2400;
        if (pass_rate >= 75) return 2200;
        if (pass_rate >= 65) return 2000;
        if (pass_rate >= 55) return 1800;
        return 1600;
    }
    
    // Generic estimate
    return static_cast<int>(1000 + pass_rate * 15);
}

} // namespace chess
