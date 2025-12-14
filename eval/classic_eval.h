#pragma once

#include "evaluator.h"

namespace chess {

// Classic evaluation using piece-square tables and material
// Used as fallback and for debugging
class ClassicEval : public Evaluator {
public:
    ClassicEval();
    ~ClassicEval() override = default;
    
    int evaluate(const Position& pos) override;
    void on_make_move(Position& pos, Move move, const UndoInfo& undo) override;
    void on_unmake_move(Position& pos, Move move, const UndoInfo& undo) override;
    void initialize(Position& pos) override;

private:
    // Piece-square tables
    static const int PST_PAWN[64];
    static const int PST_KNIGHT[64];
    static const int PST_BISHOP[64];
    static const int PST_ROOK[64];
    static const int PST_QUEEN[64];
    static const int PST_KING[64];
    static const int PST_KING_END[64];
    
    // Material values
    static const int MATERIAL[7];
    
    int pst_value(PieceType pt, Square sq, Color c) const;
    int evaluate_pieces(const Position& pos) const;
    int evaluate_pawns(const Position& pos) const;
    int evaluate_king_safety(const Position& pos) const;
};

} // namespace chess

