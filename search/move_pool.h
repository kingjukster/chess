#pragma once

#include "../board/types.h"
#include <vector>
#include <array>
#include <memory>

namespace chess {

// Memory pool for move lists to reduce allocations in search
// Pre-allocates move list buffers and reuses them
class MovePool {
public:
    // Maximum moves in a position (theoretical max is ~218)
    static constexpr size_t MAX_MOVES = 256;
    
    // Move list with fixed capacity
    class MoveList {
    public:
        MoveList() : size_(0) {}
        
        void clear() { size_ = 0; }
        void push_back(Move move) {
            if (size_ < MAX_MOVES) {
                moves_[size_++] = move;
            }
        }
        
        Move& operator[](size_t idx) { return moves_[idx]; }
        const Move& operator[](size_t idx) const { return moves_[idx]; }
        
        Move* begin() { return moves_.data(); }
        Move* end() { return moves_.data() + size_; }
        const Move* begin() const { return moves_.data(); }
        const Move* end() const { return moves_.data() + size_; }
        
        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        
        // Allow sorting
        void sort(auto compare_fn) {
            std::sort(begin(), end(), compare_fn);
        }
        
    private:
        std::array<Move, MAX_MOVES> moves_;
        size_t size_;
    };
    
    MovePool() : next_free_(0) {
        // Pre-allocate move lists for each ply
        for (size_t i = 0; i < MAX_PLY; i++) {
            pools_[i] = std::make_unique<MoveList>();
        }
    }
    
    // Get a move list for a given ply
    MoveList* get(size_t ply) {
        if (ply >= MAX_PLY) {
            // Fallback for deep positions
            return &overflow_;
        }
        pools_[ply]->clear();
        return pools_[ply].get();
    }
    
    // Reset pool (call at start of search)
    void reset() {
        next_free_ = 0;
        overflow_.clear();
    }
    
private:
    static constexpr size_t MAX_PLY = 256;
    std::array<std::unique_ptr<MoveList>, MAX_PLY> pools_;
    size_t next_free_;
    MoveList overflow_; // Fallback for overflow
};

} // namespace chess
