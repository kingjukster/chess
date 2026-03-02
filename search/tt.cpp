#include "tt.h"
#include <algorithm>
#include <cmath>

namespace chess {

TranspositionTable::TranspositionTable() 
    : num_entries_(0), size_mb_(0), generation(0), probes_(0), hits_(0) {
    // Default size: 128 MB
    resize(128);
}

TranspositionTable::~TranspositionTable() {
}

void TranspositionTable::resize(size_t mb) {
    // Calculate number of entries (each entry is 16 bytes)
    size_t bytes = mb * 1024 * 1024;
    size_t entries = bytes / sizeof(TTEntry);
    
    // Round down to nearest power of 2 for fast modulo
    entries = 1ULL << static_cast<size_t>(std::log2(entries));
    
    // Allocate new table
    table_ = std::make_unique<TTEntry[]>(entries);
    num_entries_ = entries;
    size_mb_ = mb;
    
    clear();
}

void TranspositionTable::clear() {
    if (table_) {
        std::memset(table_.get(), 0, num_entries_ * sizeof(TTEntry));
    }
    generation = 0;
    probes_ = 0;
    hits_ = 0;
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta,
                                int& score, Move& best_move, TTFlag& flag) const {
    if (!table_) return false;
    
    score = 0;
    best_move = MOVE_NONE;
    flag = TT_NONE;
    
    probes_++;
    
    size_t idx = index(key);
    const TTEntry& entry = table_[idx];
    
    // Check if entry matches our position
    if (entry.key != key) {
        return false;
    }
    
    hits_++;
    
    // Always return the best move, even if depth is insufficient
    best_move = entry.best_move;
    flag = static_cast<TTFlag>(entry.flag);
    score = entry.score;
    
    // Check if we can use this entry's score
    if (entry.depth < depth) {
        return false; // Not searched deep enough
    }
    
    // Check bound type
    switch (entry.flag) {
        case TT_EXACT:
            return true; // Exact score, always usable
            
        case TT_ALPHA:
            // Upper bound - score <= entry.score
            if (entry.score <= alpha) {
                return true;
            }
            break;
            
        case TT_BETA:
            // Lower bound - score >= entry.score
            if (entry.score >= beta) {
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int score, 
                                Move best_move, TTFlag flag) {
    if (!table_) return;
    
    size_t idx = index(key);
    TTEntry& entry = table_[idx];
    
    // Replacement strategy
    if (entry.key != 0 && entry.key != key) {
        if (!should_replace(entry, depth, generation)) {
            return; // Don't replace this entry
        }
    }
    
    // Store new entry
    entry.key = key;
    entry.score = static_cast<int16_t>(score);
    entry.best_move = best_move;
    entry.depth = static_cast<int8_t>(depth);
    entry.flag = flag;
    entry.age = generation;
}

bool TranspositionTable::should_replace(const TTEntry& existing, 
                                         int new_depth, uint8_t new_age) const {
    // Always replace if from older search
    if (existing.age != new_age) {
        return true;
    }
    
    // Replace if new entry is deeper
    if (new_depth > existing.depth) {
        return true;
    }
    
    // Keep existing entry
    return false;
}

double TranspositionTable::hit_rate() const {
    uint64_t p = probes_.load();
    uint64_t h = hits_.load();
    
    if (p == 0) return 0.0;
    return static_cast<double>(h) / static_cast<double>(p);
}

int TranspositionTable::hashfull() const {
    if (!table_ || num_entries_ == 0) return 0;
    
    // Sample first 1000 entries to estimate fullness
    size_t sample_size = std::min(size_t(1000), num_entries_);
    size_t filled = 0;
    
    for (size_t i = 0; i < sample_size; i++) {
        if (table_[i].key != 0 && table_[i].age == generation) {
            filled++;
        }
    }
    
    // Return per mille (0-1000)
    return static_cast<int>((filled * 1000) / sample_size);
}

} // namespace chess
