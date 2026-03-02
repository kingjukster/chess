#pragma once

#include "epd_parser.h"
#include "../search/search.h"
#include <string>
#include <vector>

namespace chess {

struct EpdTestResult {
    int total_positions;
    int passed;
    int failed;
    double pass_rate;
    int total_time_ms;
    std::vector<std::string> failed_ids;
    
    EpdTestResult() : total_positions(0), passed(0), failed(0), 
                      pass_rate(0.0), total_time_ms(0) {}
};

class EpdRunner {
public:
    EpdRunner(Search* search);
    
    // Run EPD test suite
    EpdTestResult run_suite(const std::string& filename, int depth = 10, int time_limit_ms = 5000);
    
    // Run single EPD position
    bool test_position(const EpdEntry& entry, int depth = 10, int time_limit_ms = 5000);
    
    // Print test results
    static void print_results(const EpdTestResult& result);
    
    // Estimate ELO from pass rate (rough approximation)
    static int estimate_elo(double pass_rate, const std::string& suite_name);
    
private:
    Search* search;
    
    // Check if move matches any of the expected moves
    bool is_correct_move(Move move, const std::vector<std::string>& expected_moves, const Position& pos);
    
    // Convert move to string
    std::string move_to_string(Move move);
};

} // namespace chess
