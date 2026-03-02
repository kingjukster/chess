#pragma once

#include "../board/position.h"
#include <string>
#include <vector>
#include <map>

namespace chess {

// EPD (Extended Position Description) entry
struct EpdEntry {
    std::string fen;
    std::vector<std::string> best_moves;    // bm (best move)
    std::vector<std::string> avoid_moves;   // am (avoid move)
    std::string id;                          // id
    std::string description;                 // c0 (comment)
    int target_depth;                        // acd (analysis count depth)
    int target_time;                         // acn (analysis count nodes) or time
    std::map<std::string, std::string> operations;  // All operations
    
    EpdEntry() : target_depth(0), target_time(0) {}
};

class EpdParser {
public:
    // Parse a single EPD line
    static EpdEntry parse_line(const std::string& line);
    
    // Load EPD file
    static std::vector<EpdEntry> load_file(const std::string& filename);
    
    // Parse FEN part (first 4 fields)
    static std::string extract_fen(const std::string& epd_line);
    
    // Parse operations (key-value pairs after FEN)
    static std::map<std::string, std::string> parse_operations(const std::string& ops_string);
    
    // Parse move list (for bm/am operations)
    static std::vector<std::string> parse_move_list(const std::string& move_string);
};

} // namespace chess
