#pragma once

#include "../board/types.h"
#include "../board/position.h"

namespace chess {

// Base evaluator interface
// All evaluators must implement this to support incremental NNUE updates
class Evaluator {
public:
    virtual ~Evaluator() = default;
    
    // Main evaluation function
    virtual int evaluate(const Position& pos) = 0;
    
    // Incremental update hooks (called by Position::make_move/unmake_move)
    virtual void on_make_move(Position& pos, Move move, const UndoInfo& undo) = 0;
    virtual void on_unmake_move(Position& pos, Move move, const UndoInfo& undo) = 0;
    
    // Initialize evaluator for a position (called when position is set up)
    virtual void initialize(Position& pos) = 0;
};

} // namespace chess

