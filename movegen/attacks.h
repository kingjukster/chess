#pragma once

#include "../board/types.h"

namespace chess {

class Position;  // Forward declaration

// Attack tables and magic bitboards for sliding pieces
class Attacks {
public:
    static void init();
    
    // Pawn attacks
    static Bitboard pawn_attacks(Square sq, Color c);
    
    // Knight attacks
    static Bitboard knight_attacks(Square sq);
    
    // King attacks
    static Bitboard king_attacks(Square sq);
    
    // Bishop attacks (with occupancy)
    static Bitboard bishop_attacks(Square sq, Bitboard occupancy);
    
    // Rook attacks (with occupancy)
    static Bitboard rook_attacks(Square sq, Bitboard occupancy);
    
    // Queen attacks (with occupancy)
    static Bitboard queen_attacks(Square sq, Bitboard occupancy);
    
    // All attacks from a square
    static Bitboard attacks_from(PieceType pt, Square sq, Bitboard occupancy = 0);
    
    // Check if square is attacked by color
    static bool is_attacked(const Position& pos, Square sq, Color by_color);
    
    // Get bitboard of all pieces attacking a square (for SEE)
    static Bitboard attackers_to(const Position& pos, Square sq, Color by_color);
    
    // Check if king is in check
    static bool in_check(const Position& pos, Color c);

private:
    // Magic bitboard tables
    static Bitboard rook_magics[64];
    static Bitboard bishop_magics[64];
    static int rook_shifts[64];
    static int bishop_shifts[64];
    static Bitboard* rook_table;
    static Bitboard* bishop_table;
    
    // Precomputed attack tables
    static Bitboard pawn_attacks_table[2][64];
    static Bitboard knight_attacks_table[64];
    static Bitboard king_attacks_table[64];
    
    // Magic bitboard helpers
    static Bitboard generate_rook_attacks(Square sq, Bitboard occupancy);
    static Bitboard generate_bishop_attacks(Square sq, Bitboard occupancy);
    static Bitboard find_magic(Square sq, int relevant_bits, bool is_rook);
    static void init_magics();
    static void init_sliders();
    static void init_non_sliders();
};

} // namespace chess

