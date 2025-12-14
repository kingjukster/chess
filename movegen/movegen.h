#pragma once

#include "../board/types.h"
#include "../board/position.h"
#include <vector>

namespace chess {

// Move generation
class MoveGen {
public:
    // Generate all pseudo-legal moves
    static void generate_moves(const Position& pos, std::vector<Move>& moves);
    
    // Generate only captures
    static void generate_captures(const Position& pos, std::vector<Move>& moves);
    
    // Generate only quiet moves
    static void generate_quiet(const Position& pos, std::vector<Move>& moves);
    
    // Generate legal moves (filtered)
    static void generate_legal(const Position& pos, std::vector<Move>& moves);
    
    // Check if move is legal
    static bool is_legal(const Position& pos, Move move);
    
    // Perft for move generation testing
    static uint64_t perft(const Position& pos, int depth);

private:
    static void generate_pawn_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_knight_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_bishop_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_rook_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_queen_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_king_moves(const Position& pos, std::vector<Move>& moves);
    static void generate_castling(const Position& pos, std::vector<Move>& moves);
};

} // namespace chess

