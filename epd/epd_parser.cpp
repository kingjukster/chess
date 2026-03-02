#include "epd_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace chess {

EpdEntry EpdParser::parse_line(const std::string& line) {
    EpdEntry entry;
    
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
        return entry;
    }
    
    // Extract FEN (first 4 fields)
    entry.fen = extract_fen(line);
    
    // Find where operations start (after 4th space)
    size_t pos = 0;
    int space_count = 0;
    for (size_t i = 0; i < line.length(); i++) {
        if (line[i] == ' ') {
            space_count++;
            if (space_count == 4) {
                pos = i + 1;
                break;
            }
        }
    }
    
    if (pos > 0 && pos < line.length()) {
        std::string ops_string = line.substr(pos);
        entry.operations = parse_operations(ops_string);
        
        // Extract common operations
        if (entry.operations.count("bm")) {
            entry.best_moves = parse_move_list(entry.operations["bm"]);
        }
        if (entry.operations.count("am")) {
            entry.avoid_moves = parse_move_list(entry.operations["am"]);
        }
        if (entry.operations.count("id")) {
            entry.id = entry.operations["id"];
            // Remove quotes if present
            if (!entry.id.empty() && entry.id.front() == '"') {
                entry.id = entry.id.substr(1, entry.id.length() - 2);
            }
        }
        if (entry.operations.count("c0")) {
            entry.description = entry.operations["c0"];
            // Remove quotes if present
            if (!entry.description.empty() && entry.description.front() == '"') {
                entry.description = entry.description.substr(1, entry.description.length() - 2);
            }
        }
        if (entry.operations.count("acd")) {
            entry.target_depth = std::stoi(entry.operations["acd"]);
        }
    }
    
    return entry;
}

std::vector<EpdEntry> EpdParser::load_file(const std::string& filename) {
    std::vector<EpdEntry> entries;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open EPD file " << filename << std::endl;
        return entries;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        EpdEntry entry = parse_line(line);
        if (!entry.fen.empty()) {
            entries.push_back(entry);
        }
    }
    
    return entries;
}

std::string EpdParser::extract_fen(const std::string& epd_line) {
    std::istringstream iss(epd_line);
    std::string fen;
    std::string token;
    
    // Read first 4 fields (board, side, castling, ep)
    for (int i = 0; i < 4; i++) {
        if (!(iss >> token)) break;
        if (i > 0) fen += " ";
        fen += token;
    }
    
    // Add default halfmove and fullmove counters
    fen += " 0 1";
    
    return fen;
}

std::map<std::string, std::string> EpdParser::parse_operations(const std::string& ops_string) {
    std::map<std::string, std::string> operations;
    
    size_t pos = 0;
    while (pos < ops_string.length()) {
        // Skip whitespace
        while (pos < ops_string.length() && std::isspace(ops_string[pos])) {
            pos++;
        }
        if (pos >= ops_string.length()) break;
        
        // Read operation code
        size_t op_start = pos;
        while (pos < ops_string.length() && !std::isspace(ops_string[pos])) {
            pos++;
        }
        std::string op_code = ops_string.substr(op_start, pos - op_start);
        
        // Skip whitespace
        while (pos < ops_string.length() && std::isspace(ops_string[pos])) {
            pos++;
        }
        
        // Read operand (until semicolon)
        std::string operand;
        bool in_quotes = false;
        while (pos < ops_string.length()) {
            char c = ops_string[pos];
            if (c == '"') {
                in_quotes = !in_quotes;
                operand += c;
            } else if (c == ';' && !in_quotes) {
                pos++;
                break;
            } else {
                operand += c;
            }
            pos++;
        }
        
        // Trim operand
        size_t start = operand.find_first_not_of(" \t");
        size_t end = operand.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            operand = operand.substr(start, end - start + 1);
        }
        
        operations[op_code] = operand;
    }
    
    return operations;
}

std::vector<std::string> EpdParser::parse_move_list(const std::string& move_string) {
    std::vector<std::string> moves;
    std::istringstream iss(move_string);
    std::string move;
    
    while (iss >> move) {
        moves.push_back(move);
    }
    
    return moves;
}

} // namespace chess
