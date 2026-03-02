#pragma once

#include "../board/types.h"
#include <atomic>
#include <cstring>
#include <memory>

namespace chess {

// TT entry flags
enum TTFlag : uint8_t {
    TT_NONE = 0,
    TT_EXACT = 1,
    TT_ALPHA = 2,
    TT_BETA = 3
};

// Transposition table entry (16 bytes, cache-line friendly)
struct alignas(16) TTEntry {
    uint64_t key;           // 8 bytes - Zobrist key
    int16_t score;          // 2 bytes - Score
    Move best_move;         // 2 bytes - Best move
    int8_t depth;           // 1 byte - Search depth
    uint8_t flag;           // 1 byte - Bound type (exact/alpha/beta)
    uint8_t age;            // 1 byte - Generation for aging
    uint8_t padding;        // 1 byte - Padding to 16 bytes
    
    TTEntry() : key(0), score(0), best_move(MOVE_NONE), depth(0), 
                flag(TT_NONE), age(0), padding(0) {}
};

// Thread-safe transposition table with lock-free access
class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();
    
    // Initialize with size in MB
    void resize(size_t mb);
    
    // Clear all entries
    void clear();
    
    // Probe the TT for a position
    // Returns true if entry found and usable for cutoff
    // On key match, always fills best_move (for move ordering) even when returning false
    bool probe(uint64_t key, int depth, int alpha, int beta, 
               int& score, Move& best_move, TTFlag& flag) const;
    
    // Store an entry in the TT
    void store(uint64_t key, int depth, int score, Move best_move, TTFlag flag);
    
    // Get TT hit rate (for statistics)
    double hit_rate() const;
    
    // Increment generation (for aging)
    void new_search() { generation++; }
    
    // Get current size in MB
    size_t size_mb() const { return size_mb_; }
    
    // Get number of entries
    size_t num_entries() const { return num_entries_; }
    
    // Get hashfull (per mille - 0-1000)
    int hashfull() const;

private:
    // Get index from key
    size_t index(uint64_t key) const { return key & (num_entries_ - 1); }
    
    // Replacement strategy: prefer entries with lower depth and older age
    bool should_replace(const TTEntry& existing, int new_depth, uint8_t new_age) const;
    
    std::unique_ptr<TTEntry[]> table_;
    size_t num_entries_;
    size_t size_mb_;
    uint8_t generation;
    
    // Statistics (atomic for thread safety)
    mutable std::atomic<uint64_t> probes_;
    mutable std::atomic<uint64_t> hits_;
};

} // namespace chess
