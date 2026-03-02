#pragma once

#include <cstdint>
#include "../board/types.h"
#include "../board/position.h"
#include <string>
#include <vector>

namespace chess {

// Tablebase probe results
enum TBResult : int {
    TB_LOSS = 0,
    TB_BLESSED_LOSS = 1,  // Loss but 50-move rule draw possible
    TB_DRAW = 2,
    TB_CURSED_WIN = 3,    // Win but 50-move rule draw possible
    TB_WIN = 4,
    TB_FAILED = 5         // Probe failed (position not in TB)
};

// Tablebase move with WDL and DTZ information
struct TBMove {
    Move move;
    TBResult wdl;      // Win/Draw/Loss
    int dtz;           // Distance to zeroing move (or mate)
    
    TBMove() : move(MOVE_NONE), wdl(TB_FAILED), dtz(0) {}
    TBMove(Move m, TBResult w, int d) : move(m), wdl(w), dtz(d) {}
};

struct SyzygyTablebase {
public:
    SyzygyTablebase();
    ~SyzygyTablebase();
    
    // Initialize tablebase with path to TB files
    bool init(const std::string& path);
    
    // Close tablebase
    void close();
    
    // Check if tablebase is initialized
    bool is_initialized() const { return initialized; }
    
    // Get maximum number of pieces in loaded tablebases
    int max_pieces() const { return max_cardinality; }
    
    // Probe WDL (Win/Draw/Loss) at root
    TBResult probe_wdl(const Position& pos);
    
    // Probe DTZ (Distance to Zero) at root
    int probe_dtz(const Position& pos);
    
    // Get best move from tablebase (root position)
    Move probe_root(const Position& pos, TBResult& wdl, int& dtz);
    
    // Probe during search (returns score)
    bool probe_search(const Position& pos, int& score);
    
    // Get all legal moves with TB info
    std::vector<TBMove> probe_root_moves(const Position& pos);
    
    // Settings
    void set_probe_depth(int depth) { probe_depth = depth; }
    void set_probe_limit(int pieces) { probe_limit = pieces; }
    int get_probe_depth() const { return probe_depth; }
    
    // Statistics
    uint64_t get_hits() const { return tb_hits; }
    void reset_stats() { tb_hits = 0; }
    
private:
    bool initialized;
    std::string tb_path;
    int max_cardinality;     // Maximum pieces in loaded TBs
    int probe_depth;         // Minimum depth to probe
    int probe_limit;         // Maximum pieces to probe
    uint64_t tb_hits;        // Number of successful probes
    
    // Convert position to Fathom format (bitboards as uint64_t, ep as square index 0-63)
    int fathom_position(const Position& pos, uint64_t* white, uint64_t* black,
                       uint64_t* kings, uint64_t* queens, uint64_t* rooks,
                       uint64_t* bishops, uint64_t* knights, uint64_t* pawns,
                       unsigned* ep, bool* turn) const;
    
    // Convert Fathom move to engine move
    Move fathom_to_move(unsigned fathom_move) const;
    
    // Convert engine move to Fathom format
    unsigned move_to_fathom(Move move) const;
    
    // Count pieces on board
    int count_pieces(const Position& pos) const;
    
    // Convert WDL to score
    int wdl_to_score(TBResult wdl, int ply) const;
};

} // namespace chess
