#include "attacks.h"
#include "../board/position.h"
#include "../board/bitboard.h"
#include <random>
#include <cstring>

namespace chess {

// Static member initialization
Bitboard Attacks::rook_magics[64];
Bitboard Attacks::bishop_magics[64];
int Attacks::rook_shifts[64];
int Attacks::bishop_shifts[64];
Bitboard* Attacks::rook_table = nullptr;
Bitboard* Attacks::bishop_table = nullptr;
Bitboard Attacks::pawn_attacks_table[2][64];
Bitboard Attacks::knight_attacks_table[64];
Bitboard Attacks::king_attacks_table[64];

// Precomputed attack masks
static Bitboard rook_masks[64];
static Bitboard bishop_masks[64];

// Direction vectors
static const int RookDirections[4] = {8, -8, 1, -1};
static const int BishopDirections[4] = {9, -9, 7, -7};

void Attacks::init() {
    init_non_sliders();
    init_sliders();
}

void Attacks::init_non_sliders() {
    // Pawn attacks
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard bb = 1ULL << sq;
        pawn_attacks_table[WHITE][sq] = ((bb << 9) & ~0x0101010101010101ULL) | ((bb << 7) & ~0x8080808080808080ULL);
        pawn_attacks_table[BLACK][sq] = ((bb >> 9) & ~0x8080808080808080ULL) | ((bb >> 7) & ~0x0101010101010101ULL);
    }
    
    // Knight attacks
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard bb = 1ULL << sq;
        knight_attacks_table[sq] = 
            ((bb << 17) & ~0x0101010101010101ULL) | ((bb << 15) & ~0x8080808080808080ULL) |
            ((bb << 10) & ~0x0303030303030303ULL) | ((bb << 6) & ~0xC0C0C0C0C0C0C0C0ULL) |
            ((bb >> 17) & ~0x8080808080808080ULL) | ((bb >> 15) & ~0x0101010101010101ULL) |
            ((bb >> 10) & ~0xC0C0C0C0C0C0C0C0ULL) | ((bb >> 6) & ~0x0303030303030303ULL);
    }
    
    // King attacks
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard bb = 1ULL << sq;
        king_attacks_table[sq] = 
            ((bb << 8) | (bb >> 8)) |
            (((bb & ~0x8080808080808080ULL) << 1) | ((bb & ~0x0101010101010101ULL) >> 1)) |
            (((bb & ~0x8080808080808080ULL) << 9) | ((bb & ~0x0101010101010101ULL) >> 9)) |
            (((bb & ~0x0101010101010101ULL) << 7) | ((bb & ~0x8080808080808080ULL) >> 7));
    }
}

Bitboard Attacks::generate_rook_attacks(Square sq, Bitboard occupancy) {
    Bitboard attacks = 0;
    int rank = rank_of(sq);
    int file = file_of(sq);
    
    // North
    for (int r = rank + 1; r < 8; r++) {
        Square s = make_square(file, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // South
    for (int r = rank - 1; r >= 0; r--) {
        Square s = make_square(file, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // East
    for (int f = file + 1; f < 8; f++) {
        Square s = make_square(f, rank);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // West
    for (int f = file - 1; f >= 0; f--) {
        Square s = make_square(f, rank);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    
    return attacks;
}

Bitboard Attacks::generate_bishop_attacks(Square sq, Bitboard occupancy) {
    Bitboard attacks = 0;
    int rank = rank_of(sq);
    int file = file_of(sq);
    
    // Northeast
    for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
        Square s = make_square(f, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // Northwest
    for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
        Square s = make_square(f, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // Southeast
    for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
        Square s = make_square(f, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    // Southwest
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        Square s = make_square(f, r);
        attacks |= 1ULL << s;
        if (occupancy & (1ULL << s)) break;
    }
    
    return attacks;
}

void Attacks::init_sliders() {
    // Initialize masks
    for (Square sq = 0; sq < 64; sq++) {
        int rank = rank_of(sq);
        int file = file_of(sq);
        
        // Rook mask: exclude edge ranks and files
        Bitboard rook_mask = 0;
        // North (exclude rank 7)
        for (int r = rank + 1; r < 7; r++) {
            rook_mask |= 1ULL << make_square(file, r);
        }
        // South (exclude rank 0)
        for (int r = rank - 1; r > 0; r--) {
            rook_mask |= 1ULL << make_square(file, r);
        }
        // East (exclude file 7)
        for (int f = file + 1; f < 7; f++) {
            rook_mask |= 1ULL << make_square(f, rank);
        }
        // West (exclude file 0)
        for (int f = file - 1; f > 0; f--) {
            rook_mask |= 1ULL << make_square(f, rank);
        }
        rook_masks[sq] = rook_mask;
        
        // Bishop mask: exclude edge squares
        Bitboard bishop_mask = 0;
        // Northeast (exclude edges)
        for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++) {
            bishop_mask |= 1ULL << make_square(f, r);
        }
        // Northwest (exclude edges)
        for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--) {
            bishop_mask |= 1ULL << make_square(f, r);
        }
        // Southeast (exclude edges)
        for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++) {
            bishop_mask |= 1ULL << make_square(f, r);
        }
        // Southwest (exclude edges)
        for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--) {
            bishop_mask |= 1ULL << make_square(f, r);
        }
        bishop_masks[sq] = bishop_mask;
        
        rook_shifts[sq] = 64 - popcount(rook_masks[sq]);
        bishop_shifts[sq] = 64 - popcount(bishop_masks[sq]);
    }
    
    // Allocate tables (simplified - would use proper sizing in production)
    rook_table = new Bitboard[64 * 4096];
    bishop_table = new Bitboard[64 * 512];
    
    // Initialize magics (simplified - would use proper magic numbers)
    std::mt19937_64 rng(12345);
    for (Square sq = 0; sq < 64; sq++) {
        rook_magics[sq] = rng() & rng() & rng();
        bishop_magics[sq] = rng() & rng() & rng();
    }
    
    // Fill tables
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard mask = rook_masks[sq];
        int n = popcount(mask);
        Bitboard subset = 0;
        
        do {
            Bitboard attacks = generate_rook_attacks(sq, subset);
            int idx = ((subset * rook_magics[sq]) >> rook_shifts[sq]) + (sq * 4096);
            rook_table[idx] = attacks;
            subset = (subset - mask) & mask;
        } while (subset);
    }
    
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard mask = bishop_masks[sq];
        int n = popcount(mask);
        Bitboard subset = 0;
        
        do {
            Bitboard attacks = generate_bishop_attacks(sq, subset);
            int idx = ((subset * bishop_magics[sq]) >> bishop_shifts[sq]) + (sq * 512);
            bishop_table[idx] = attacks;
            subset = (subset - mask) & mask;
        } while (subset);
    }
}

Bitboard Attacks::pawn_attacks(Square sq, Color c) {
    return pawn_attacks_table[c][sq];
}

Bitboard Attacks::knight_attacks(Square sq) {
    return knight_attacks_table[sq];
}

Bitboard Attacks::king_attacks(Square sq) {
    return king_attacks_table[sq];
}

Bitboard Attacks::bishop_attacks(Square sq, Bitboard occupancy) {
    return generate_bishop_attacks(sq, occupancy);
}

Bitboard Attacks::rook_attacks(Square sq, Bitboard occupancy) {
    return generate_rook_attacks(sq, occupancy);
}

Bitboard Attacks::queen_attacks(Square sq, Bitboard occupancy) {
    return bishop_attacks(sq, occupancy) | rook_attacks(sq, occupancy);
}

Bitboard Attacks::attacks_from(PieceType pt, Square sq, Bitboard occupancy) {
    switch (pt) {
        case PAWN: return 0; // Need color
        case KNIGHT: return knight_attacks(sq);
        case BISHOP: return bishop_attacks(sq, occupancy);
        case ROOK: return rook_attacks(sq, occupancy);
        case QUEEN: return queen_attacks(sq, occupancy);
        case KING: return king_attacks(sq);
        default: return 0;
    }
}

bool Attacks::is_attacked(const Position& pos, Square sq, Color by_color) {
    Bitboard occupancy = pos.pieces(WHITE) | pos.pieces(BLACK);
    
    // Pawns
    Bitboard pawn_attackers = pawn_attacks(sq, static_cast<Color>(1 - by_color)) & pos.pieces(by_color, PAWN);
    if (pawn_attackers) {
        return true;
    }
    
    // Knights
    Bitboard knight_attackers = knight_attacks(sq) & pos.pieces(by_color, KNIGHT);
    if (knight_attackers) {
        return true;
    }
    
    // Bishops and Queens (diagonal attacks)
    Bitboard bishop_attackers = bishop_attacks(sq, occupancy) & (pos.pieces(by_color, BISHOP) | pos.pieces(by_color, QUEEN));
    if (bishop_attackers) {
        return true;
    }
    
    // Rooks and Queens (straight attacks)
    Bitboard rook_attackers = rook_attacks(sq, occupancy) & (pos.pieces(by_color, ROOK) | pos.pieces(by_color, QUEEN));
    if (rook_attackers) {
        return true;
    }
    
    // King
    Bitboard king_attackers = king_attacks(sq) & pos.pieces(by_color, KING);
    if (king_attackers) {
        return true;
    }
    
    return false;
}

Bitboard Attacks::attackers_to(const Position& pos, Square sq, Color by_color) {
    Bitboard occupancy = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard attackers = 0;
    attackers |= pawn_attacks(sq, static_cast<Color>(1 - by_color)) & pos.pieces(by_color, PAWN);
    attackers |= knight_attacks(sq) & pos.pieces(by_color, KNIGHT);
    attackers |= bishop_attacks(sq, occupancy) & (pos.pieces(by_color, BISHOP) | pos.pieces(by_color, QUEEN));
    attackers |= rook_attacks(sq, occupancy) & (pos.pieces(by_color, ROOK) | pos.pieces(by_color, QUEEN));
    attackers |= king_attacks(sq) & pos.pieces(by_color, KING);
    return attackers;
}

bool Attacks::in_check(const Position& pos, Color c) {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return false;
    return is_attacked(pos, king_sq, static_cast<Color>(1 - c));
}

} // namespace chess

