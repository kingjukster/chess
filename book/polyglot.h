#pragma once

#include "../board/types.h"
#include "../board/position.h"
#include <string>
#include <vector>
#include <fstream>
#include <random>

namespace chess {

// Polyglot book entry (16 bytes)
struct PolyglotEntry {
    uint64_t key;      // Position hash
    uint16_t move;     // Move in Polyglot format
    uint16_t weight;   // Move weight
    uint32_t learn;    // Learning data (unused)
    
    PolyglotEntry() : key(0), move(0), weight(0), learn(0) {}
};

// Book move with weight
struct BookMove {
    Move move;
    uint16_t weight;
    
    BookMove(Move m, uint16_t w) : move(m), weight(w) {}
};

class PolyglotBook {
public:
    PolyglotBook();
    ~PolyglotBook();
    
    // Load opening book from file
    bool load(const std::string& filename);
    
    // Close book
    void close();
    
    // Check if book is loaded
    bool is_loaded() const { return file.is_open(); }
    
    // Probe book for position
    std::vector<BookMove> probe(const Position& pos);
    
    // Get best move from book
    Move get_move(const Position& pos, bool best = false);
    
    // Settings
    void set_variety(int v) { variety = v; }  // 0-100, higher = more variety
    void set_min_weight(int w) { min_weight = w; }
    void set_max_depth(int d) { max_book_depth = d; }
    
    int get_book_depth() const { return book_ply; }
    void reset_ply() { book_ply = 0; }  // Reset book ply counter for new game
    
private:
    std::ifstream file;
    std::string filename;
    size_t num_entries;
    
    // Settings
    int variety;        // 0-100, controls move selection randomness
    int min_weight;     // Minimum weight for move consideration
    int max_book_depth; // Maximum ply to use book
    int book_ply;       // Current book ply counter
    
    // Random number generator
    std::mt19937_64 rng;
    
    // Polyglot zobrist keys (different from engine's internal keys)
    uint64_t polyglot_zobrist_key(const Position& pos) const;
    
    // Convert Polyglot move format to engine Move
    Move polyglot_to_move(const Position& pos, uint16_t poly_move) const;
    
    // Convert engine Move to Polyglot format
    uint16_t move_to_polyglot(const Position& pos, Move move) const;
    
    // Binary search for position in book
    std::vector<PolyglotEntry> find_entries(uint64_t key);
    
    // Polyglot zobrist tables
    static const uint64_t POLYGLOT_RANDOM[781];
};

} // namespace chess
