#pragma once

#include "../board/position.h"
#include "../search/search.h"
#include "../eval/evaluator.h"
#include "../eval/tuner.h"
#include "../book/polyglot.h"
#include "../tablebase/syzygy.h"
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
    void handle_bench(const std::string& line);
    void handle_display();
    void handle_tune(const std::string& line);
    void handle_export_params(const std::string& line);
    Move parse_move(const std::string& move_str);
    std::string move_to_string(Move move);
    
    Position pos;
    Search* search;
    Evaluator* evaluator;
    TexelTuner* tuner;
    PolyglotBook* book;
    SyzygyTablebase* tablebase;
    
    bool use_nnue;
    bool debug_mode;
    bool tuning_mode;
    std::string nnue_file;
    std::string training_data_file;
    
    // Book settings
    bool use_book;
    std::string book_file;
    int book_depth;
    int book_variety;
    bool book_best_move;
    
    // Tablebase settings
    bool use_tablebase;
    std::string tablebase_path;
    int tablebase_probe_depth;
    int tablebase_probe_limit;
    
    // Time management
    int wtime, btime, winc, binc;
    int movetime;
    int movestogo;
    int depth_limit;
    int nodes_limit;
    int mate;
    bool infinite;
};

} // namespace chess

