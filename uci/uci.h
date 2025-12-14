#pragma once

#include "../board/position.h"
#include "../search/search.h"
#include "../eval/evaluator.h"
#include <string>
#include <sstream>
#include <memory>

namespace chess {

class UCI {
public:
    UCI();
    ~UCI();
    
    void loop();
    
private:
    void handle_position(const std::string& line);
    void handle_go(const std::string& line);
    void handle_setoption(const std::string& line);
    Move parse_move(const std::string& move_str);
    std::string move_to_string(Move move);
    
    Position pos;
    Search* search;
    Evaluator* evaluator;
    bool use_nnue;
    std::string nnue_file;  // Path to NNUE network file
    
    // Time management
    int wtime, btime, winc, binc;
    int movestogo;
    int depth_limit;
    int nodes_limit;
    int mate;
    bool infinite;
};

} // namespace chess

