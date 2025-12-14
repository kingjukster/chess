#include "classic_eval.h"
#include "../board/position.h"
#include "../board/bitboard.h"

namespace chess {

// Piece-square tables (from white's perspective, flipped for black)
const int ClassicEval::PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

const int ClassicEval::PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int ClassicEval::PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int ClassicEval::PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

const int ClassicEval::PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int ClassicEval::PST_KING[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int ClassicEval::PST_KING_END[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

const int ClassicEval::MATERIAL[7] = {
    0,      // NO_PIECE
    100,    // PAWN
    320,    // KNIGHT
    330,    // BISHOP
    500,    // ROOK
    900,    // QUEEN
    20000   // KING
};

ClassicEval::ClassicEval() {
}

int ClassicEval::pst_value(PieceType pt, Square sq, Color c) const {
    int idx = (c == WHITE) ? sq : (sq ^ 56); // Flip vertically for black
    switch (pt) {
        case PAWN: return PST_PAWN[idx];
        case KNIGHT: return PST_KNIGHT[idx];
        case BISHOP: return PST_BISHOP[idx];
        case ROOK: return PST_ROOK[idx];
        case QUEEN: return PST_QUEEN[idx];
        case KING: return PST_KING[idx];
        default: return 0;
    }
}

int ClassicEval::evaluate(const Position& pos) {
    if (pos.side_to_move() == BLACK) {
        return -evaluate_pieces(pos) - evaluate_pawns(pos) - evaluate_king_safety(pos);
    }
    return evaluate_pieces(pos) + evaluate_pawns(pos) + evaluate_king_safety(pos);
}

int ClassicEval::evaluate_pieces(const Position& pos) const {
    int score = 0;
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        
        for (PieceType pt = PAWN; pt <= KING; pt = static_cast<PieceType>(pt + 1)) {
            Bitboard bb = pos.pieces(c, pt);
            while (bb) {
                Square sq = pop_lsb(bb);
                score += sign * (MATERIAL[pt] + pst_value(pt, sq, c));
            }
        }
    }
    
    return score;
}

int ClassicEval::evaluate_pawns(const Position& pos) const {
    // Simple pawn structure evaluation
    // TODO: Add doubled pawns, isolated pawns, passed pawns
    return 0;
}

int ClassicEval::evaluate_king_safety(const Position& pos) const {
    // Simple king safety
    // TODO: Add more sophisticated evaluation
    return 0;
}

void ClassicEval::on_make_move(Position& pos, Move move, const UndoInfo& undo) {
    // Classic eval doesn't need incremental updates
    (void)pos;
    (void)move;
    (void)undo;
}

void ClassicEval::on_unmake_move(Position& pos, Move move, const UndoInfo& undo) {
    // Classic eval doesn't need incremental updates
    (void)pos;
    (void)move;
    (void)undo;
}

void ClassicEval::initialize(Position& pos) {
    // Classic eval doesn't need initialization
    (void)pos;
}

} // namespace chess

