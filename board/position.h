#pragma once

#include "types.h"
#include <array>
#include <vector>
#include <string>

namespace chess {

// Forward declaration
class Evaluator;

class Position {
public:
    Position();
    Position(const std::string& fen);
    
    // Position state
    Bitboard pieces(Color c) const { return byColor[c]; }
    Bitboard pieces(PieceType pt) const { return byType[pt]; }
    Bitboard pieces(Color c, PieceType pt) const { return byColor[c] & byType[pt]; }
    Piece piece_on(Square sq) const { return board[sq]; }
    Square king_square(Color c) const { return kingSq[c]; }
    
    Color side_to_move() const { return stm; }
    int castling_rights() const { return castlingRights; }
    Square ep_square() const { return epSquare; }
    uint32_t rule50() const { return rule50Count; }
    uint64_t zobrist_key() const { return zobristKey; }
    
    // Move making
    bool make_move(Move move, UndoInfo& undo);
    void unmake_move(Move move, const UndoInfo& undo);
    
    // Position queries
    bool is_check() const;
    bool is_check(Color c) const;
    bool is_legal(Move move) const;
    bool is_draw() const;
    
    // FEN
    std::string to_fen() const;
    void from_fen(const std::string& fen);
    
    // NNUE accumulator (will be populated by NNUE evaluator)
    struct NnueState {
        // Accumulator for each side (will be managed by NNUE evaluator)
        void* acc[COLOR_NB];  // Opaque pointer to accumulator structure
        bool initialized;
        
        NnueState() : initialized(false) {
            acc[WHITE] = nullptr;
            acc[BLACK] = nullptr;
        }
    } nnue;
    
    // Evaluator hook (set by search)
    Evaluator* evaluator;

private:
    // Bitboards
    Bitboard byColor[COLOR_NB];
    Bitboard byType[KING + 1];
    Piece board[64];
    Square kingSq[COLOR_NB];
    
    // Position state
    Color stm;
    int castlingRights;
    Square epSquare;
    uint32_t rule50Count;
    uint32_t gamePly;
    uint64_t zobristKey;
    
    // Zobrist hashing
    void init_zobrist();
    uint64_t zobrist_piece[64][16];
    uint64_t zobrist_castling[16];
    uint64_t zobrist_ep[8];
    uint64_t zobrist_side;
    
    // Move making helpers
    void remove_piece(Square sq);
    void put_piece(Piece pc, Square sq);
    void move_piece(Square from, Square to);
    void update_castling_rights(Square sq);
    void update_zobrist();
};

} // namespace chess

