#pragma once

#include <cstdint>

namespace chess {

// Square representation (0-63)
using Square = int;
constexpr Square SQ_NONE = 64;

// Piece types
enum PieceType : int {
    NO_PIECE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

// Colors
enum Color : int {
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2
};

// Piece encoding: color * 8 + piece_type
enum Piece : int {
    NO_PIECE_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14
};

// Castling rights
enum CastlingRights : int {
    NO_CASTLING = 0,
    WHITE_OO = 1,
    WHITE_OOO = 2,
    BLACK_OO = 4,
    BLACK_OOO = 8,
    KING_SIDE = WHITE_OO | BLACK_OO,
    QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

// Bitboard type
using Bitboard = uint64_t;

// Move encoding: 16-bit packed format
// Bits 0-5: from square (6 bits)
// Bits 6-11: to square (6 bits)
// Bits 12-15: flags (4 bits) encoding:
//   0: normal move
//   1: capture
//   2: castling
//   3: en passant
//   4-5: knight promotion (quiet/capture)
//   6-7: bishop promotion (quiet/capture)
//   8-9: rook promotion (quiet/capture)
//   10-11: queen promotion (quiet/capture)
struct Move {
    uint16_t data;

    constexpr Move() : data(0) {}
    constexpr Move(uint16_t d) : data(d) {}
    constexpr Move(Square from, Square to, PieceType promo = NO_PIECE, bool capture = false, bool special = false)
        : data((from >= 64 || to >= 64) ? 0 : ((from) | (to << 6) | (encode_flags(promo, capture, special) << 12))) {}

    Square from() const { return data & 0x3F; }
    Square to() const { return (data >> 6) & 0x3F; }
    
    PieceType promotion() const {
        int flags = (data >> 12) & 0xF;
        if (flags >= 4 && flags <= 11) {
            return static_cast<PieceType>(KNIGHT + (flags - 4) / 2);
        }
        return NO_PIECE;
    }
    
    bool is_capture() const {
        int flags = (data >> 12) & 0xF;
        return (flags == 1) || (flags == 3) || (flags == 5) || (flags == 7) || (flags == 9) || (flags == 11);
    }
    
    bool is_special() const {
        int flags = (data >> 12) & 0xF;
        return (flags == 2) || (flags == 3);
    }
    
    bool is_promotion() const { return promotion() != NO_PIECE; }
    bool is_castling() const { return ((data >> 12) & 0xF) == 2; }
    bool is_en_passant() const { return ((data >> 12) & 0xF) == 3; }

    bool operator==(const Move& other) const { return data == other.data; }
    bool operator!=(const Move& other) const { return data != other.data; }
    bool is_valid() const { 
        // Check if the move is not a null move and squares are different
        if (data == 0) return false;
        Square f = from();
        Square t = to();
        return f != t;
    }

private:
    static constexpr int encode_flags(PieceType promo, bool capture, bool special) {
        if (promo != NO_PIECE) {
            int base = 4 + (promo - KNIGHT) * 2;
            return capture ? base + 1 : base;
        }
        if (special) {
            return capture ? 3 : 2;
        }
        return capture ? 1 : 0;
    }
};

constexpr Move MOVE_NONE(0);

// Undo information for unmake_move
// Stores all information needed to restore position and NNUE accumulator
struct UndoInfo {
    Move move;
    Piece captured_piece;
    Square captured_square;
    int castling_rights;
    Square ep_square;
    uint32_t rule50;
    uint64_t zobrist_key;
    
    // NNUE accumulator deltas
    // These track which features changed for incremental updates
    struct NnueDelta {
        Square moved_from;
        Square moved_to;
        Piece moved_piece;
        Square captured_square;
        Piece captured_piece;
        Square promo_square;
        Piece promo_piece;
        bool is_castling;
        Square rook_from;
        Square rook_to;
        bool is_en_passant;
        Square ep_captured_square;
    } nnue_delta;
};

// Square helpers
constexpr Square make_square(int file, int rank) { return rank * 8 + file; }
constexpr int file_of(Square sq) { return sq & 7; }
constexpr int rank_of(Square sq) { return sq >> 3; }

// Piece helpers
constexpr Color color_of(Piece pc) { return static_cast<Color>(pc >> 3); }
constexpr PieceType type_of(Piece pc) { return static_cast<PieceType>(pc & 7); }
constexpr Piece make_piece(Color c, PieceType pt) { return static_cast<Piece>(c * 8 + pt); }

// Castling squares
constexpr Square W_KING_START = make_square(4, 0);
constexpr Square W_ROOK_OO = make_square(7, 0);
constexpr Square W_ROOK_OOO = make_square(0, 0);
constexpr Square B_KING_START = make_square(4, 7);
constexpr Square B_ROOK_OO = make_square(7, 7);
constexpr Square B_ROOK_OOO = make_square(0, 7);

} // namespace chess

