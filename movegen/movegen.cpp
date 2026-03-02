#include "movegen.h"
#include "attacks.h"
#include "../board/bitboard.h"
#include <algorithm>

namespace chess {

void MoveGen::generate_moves(const Position& pos, std::vector<Move>& moves) {
    moves.clear();
    generate_pawn_moves(pos, moves);
    generate_knight_moves(pos, moves);
    generate_bishop_moves(pos, moves);
    generate_rook_moves(pos, moves);
    generate_queen_moves(pos, moves);
    generate_king_moves(pos, moves);
    generate_castling(pos, moves);
}

void MoveGen::generate_captures(const Position& pos, std::vector<Move>& moves) {
    moves.clear();
    Color stm = pos.side_to_move();
    Bitboard enemies = pos.pieces(static_cast<Color>(1 - stm));
    
    // Pawn captures
    Bitboard pawns = pos.pieces(stm, PAWN);
    if (stm == WHITE) {
        Bitboard captures = (pawns << 9) & ~0x0101010101010101ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to - 9;
            if (rank_of(to) == 7) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        captures = (pawns << 7) & ~0x8080808080808080ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to - 7;
            if (rank_of(to) == 7) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        // En passant
        if (pos.ep_square() != SQ_NONE) {
            Bitboard ep_attacks = Attacks::pawn_attacks(pos.ep_square(), BLACK) & pawns;
            while (ep_attacks) {
                Square from = pop_lsb(ep_attacks);
                moves.push_back(Move(from, pos.ep_square(), NO_PIECE, true, true));
            }
        }
    } else {
        Bitboard captures = (pawns >> 9) & ~0x8080808080808080ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to + 9;
            if (rank_of(to) == 0) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        captures = (pawns >> 7) & ~0x0101010101010101ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to + 7;
            if (rank_of(to) == 0) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        // En passant
        if (pos.ep_square() != SQ_NONE) {
            Bitboard ep_attacks = Attacks::pawn_attacks(pos.ep_square(), WHITE) & pawns;
            while (ep_attacks) {
                Square from = pop_lsb(ep_attacks);
                moves.push_back(Move(from, pos.ep_square(), NO_PIECE, true, true));
            }
        }
    }
    
    // Other piece captures
    generate_knight_moves(pos, moves);
    generate_bishop_moves(pos, moves);
    generate_rook_moves(pos, moves);
    generate_queen_moves(pos, moves);
    generate_king_moves(pos, moves);
    
    // Filter to only captures
    moves.erase(std::remove_if(moves.begin(), moves.end(), 
        [](const Move& m) { return !m.is_capture(); }), moves.end());
}

void MoveGen::generate_quiet(const Position& pos, std::vector<Move>& moves) {
    generate_moves(pos, moves);
    moves.erase(std::remove_if(moves.begin(), moves.end(), 
        [](const Move& m) { return m.is_capture(); }), moves.end());
}

void MoveGen::generate_legal(const Position& pos, std::vector<Move>& moves) {
    generate_moves(pos, moves);
    
    // Filter out moves that leave the king in check
    moves.erase(std::remove_if(moves.begin(), moves.end(), 
        [&pos](const Move& m) {
            Position test_pos = pos;
            UndoInfo undo;
            Color original_stm = pos.side_to_move();
            if (!test_pos.make_move(m, undo)) {
                return true; // Remove invalid moves
            }
            // After making move, side has switched, so check if original side's king is in check
            bool in_check = Attacks::in_check(test_pos, original_stm);
            return in_check; // Remove moves that leave king in check
        }), moves.end());
}

bool MoveGen::is_legal(const Position& pos, Move move) {
    // First check if the move is pseudo-legal (can be generated)
    std::vector<Move> pseudo_legal;
    generate_moves(pos, pseudo_legal);
    
    bool found = false;
    for (const Move& m : pseudo_legal) {
        // Compare from, to, promotion type, and special flags (castling, en passant)
        if (m.from() == move.from() && m.to() == move.to() && 
            m.promotion() == move.promotion() &&
            m.is_castling() == move.is_castling() &&
            m.is_en_passant() == move.is_en_passant()) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        return false;
    }
    
    // Make move and check if king is in check
    Position test_pos = pos;
    UndoInfo undo;
    Color original_stm = pos.side_to_move();
    if (!test_pos.make_move(move, undo)) {
        return false;
    }
    // After making move, side has switched, so check if original side's king is in check
    bool in_check = Attacks::in_check(test_pos, original_stm);
    test_pos.unmake_move(move, undo);
    return !in_check;
}

void MoveGen::generate_pawn_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Bitboard pawns = pos.pieces(stm, PAWN);
    Bitboard all = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard enemies = pos.pieces(static_cast<Color>(1 - stm));
    
    if (stm == WHITE) {
        // Single push
        Bitboard single = (pawns << 8) & ~all;
        while (single) {
            Square to = pop_lsb(single);
            Square from = to - 8;
            if (rank_of(to) == 7) {
                moves.push_back(Move(from, to, QUEEN));
                moves.push_back(Move(from, to, ROOK));
                moves.push_back(Move(from, to, BISHOP));
                moves.push_back(Move(from, to, KNIGHT));
            } else {
                moves.push_back(Move(from, to));
            }
        }
        // Double push
        Bitboard double_push = ((pawns & 0x000000000000FF00ULL) << 16) & ~all & ~(all << 8);
        while (double_push) {
            Square to = pop_lsb(double_push);
            moves.push_back(Move(to - 16, to));
        }
        // Pawn captures
        Bitboard captures = (pawns << 9) & ~0x0101010101010101ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to - 9;
            if (rank_of(to) == 7) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        captures = (pawns << 7) & ~0x8080808080808080ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to - 7;
            if (rank_of(to) == 7) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        // En passant
        if (pos.ep_square() != SQ_NONE) {
            Bitboard ep_attacks = Attacks::pawn_attacks(pos.ep_square(), BLACK) & pawns;
            while (ep_attacks) {
                Square from = pop_lsb(ep_attacks);
                moves.push_back(Move(from, pos.ep_square(), NO_PIECE, true, true));
            }
        }
    } else {
        // Single push
        Bitboard single = (pawns >> 8) & ~all;
        while (single) {
            Square to = pop_lsb(single);
            Square from = to + 8;
            if (rank_of(to) == 0) {
                moves.push_back(Move(from, to, QUEEN));
                moves.push_back(Move(from, to, ROOK));
                moves.push_back(Move(from, to, BISHOP));
                moves.push_back(Move(from, to, KNIGHT));
            } else {
                moves.push_back(Move(from, to));
            }
        }
        // Double push
        Bitboard double_push = ((pawns & 0x00FF000000000000ULL) >> 16) & ~all & ~(all >> 8);
        while (double_push) {
            Square to = pop_lsb(double_push);
            moves.push_back(Move(to + 16, to));
        }
        // Pawn captures
        Bitboard captures = (pawns >> 9) & ~0x8080808080808080ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to + 9;
            if (rank_of(to) == 0) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        captures = (pawns >> 7) & ~0x0101010101010101ULL & enemies;
        while (captures) {
            Square to = pop_lsb(captures);
            Square from = to + 7;
            if (rank_of(to) == 0) {
                moves.push_back(Move(from, to, QUEEN, true));
                moves.push_back(Move(from, to, ROOK, true));
                moves.push_back(Move(from, to, BISHOP, true));
                moves.push_back(Move(from, to, KNIGHT, true));
            } else {
                moves.push_back(Move(from, to, NO_PIECE, true));
            }
        }
        // En passant
        if (pos.ep_square() != SQ_NONE) {
            Bitboard ep_attacks = Attacks::pawn_attacks(pos.ep_square(), WHITE) & pawns;
            while (ep_attacks) {
                Square from = pop_lsb(ep_attacks);
                moves.push_back(Move(from, pos.ep_square(), NO_PIECE, true, true));
            }
        }
    }
}

void MoveGen::generate_knight_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Bitboard knights = pos.pieces(stm, KNIGHT);
    Bitboard friendly = pos.pieces(stm);
    
    while (knights) {
        Square from = pop_lsb(knights);
        Bitboard attacks = Attacks::knight_attacks(from) & ~friendly;
        while (attacks) {
            Square to = pop_lsb(attacks);
            bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
            moves.push_back(Move(from, to, NO_PIECE, capture));
        }
    }
}

void MoveGen::generate_bishop_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Bitboard bishops = pos.pieces(stm, BISHOP);
    Bitboard friendly = pos.pieces(stm);
    Bitboard all = pos.pieces(WHITE) | pos.pieces(BLACK);
    
    while (bishops) {
        Square from = pop_lsb(bishops);
        Bitboard attacks = Attacks::bishop_attacks(from, all) & ~friendly;
        while (attacks) {
            Square to = pop_lsb(attacks);
            bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
            moves.push_back(Move(from, to, NO_PIECE, capture));
        }
    }
}

void MoveGen::generate_rook_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Bitboard rooks = pos.pieces(stm, ROOK);
    Bitboard friendly = pos.pieces(stm);
    Bitboard all = pos.pieces(WHITE) | pos.pieces(BLACK);
    
    while (rooks) {
        Square from = pop_lsb(rooks);
        Bitboard attacks = Attacks::rook_attacks(from, all) & ~friendly;
        while (attacks) {
            Square to = pop_lsb(attacks);
            bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
            moves.push_back(Move(from, to, NO_PIECE, capture));
        }
    }
}

void MoveGen::generate_queen_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Bitboard queens = pos.pieces(stm, QUEEN);
    Bitboard friendly = pos.pieces(stm);
    Bitboard all = pos.pieces(WHITE) | pos.pieces(BLACK);
    
    while (queens) {
        Square from = pop_lsb(queens);
        Bitboard attacks = Attacks::queen_attacks(from, all) & ~friendly;
        while (attacks) {
            Square to = pop_lsb(attacks);
            bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
            moves.push_back(Move(from, to, NO_PIECE, capture));
        }
    }
}

void MoveGen::generate_king_moves(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Square king_sq = pos.king_square(stm);
    if (king_sq == SQ_NONE) return;
    
    Bitboard friendly = pos.pieces(stm);
    Bitboard attacks = Attacks::king_attacks(king_sq) & ~friendly;
    
    while (attacks) {
        Square to = pop_lsb(attacks);
        bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
        moves.push_back(Move(king_sq, to, NO_PIECE, capture));
    }
}

void MoveGen::generate_castling(const Position& pos, std::vector<Move>& moves) {
    Color stm = pos.side_to_move();
    Square king_sq = pos.king_square(stm);
    if (king_sq == SQ_NONE) return;
    
    Bitboard all = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard enemies = pos.pieces(static_cast<Color>(1 - stm));
    
    // Check if in check
    if (Attacks::in_check(pos, stm)) return;
    
    if (stm == WHITE) {
        // O-O
        if ((pos.castling_rights() & WHITE_OO) &&
            king_sq == W_KING_START &&
            !(all & (1ULL << make_square(5, 0) | 1ULL << make_square(6, 0))) &&
            !Attacks::is_attacked(pos, make_square(5, 0), BLACK) &&
            !Attacks::is_attacked(pos, make_square(6, 0), BLACK)) {
            moves.push_back(Move(king_sq, make_square(6, 0), NO_PIECE, false, true));
        }
        // O-O-O
        if ((pos.castling_rights() & WHITE_OOO) &&
            king_sq == W_KING_START &&
            !(all & (1ULL << make_square(1, 0) | 1ULL << make_square(2, 0) | 1ULL << make_square(3, 0))) &&
            !Attacks::is_attacked(pos, make_square(2, 0), BLACK) &&
            !Attacks::is_attacked(pos, make_square(3, 0), BLACK)) {
            moves.push_back(Move(king_sq, make_square(2, 0), NO_PIECE, false, true));
        }
    } else {
        // O-O
        if ((pos.castling_rights() & BLACK_OO) &&
            king_sq == B_KING_START &&
            !(all & (1ULL << make_square(5, 7) | 1ULL << make_square(6, 7))) &&
            !Attacks::is_attacked(pos, make_square(5, 7), WHITE) &&
            !Attacks::is_attacked(pos, make_square(6, 7), WHITE)) {
            moves.push_back(Move(king_sq, make_square(6, 7), NO_PIECE, false, true));
        }
        // O-O-O
        if ((pos.castling_rights() & BLACK_OOO) &&
            king_sq == B_KING_START &&
            !(all & (1ULL << make_square(1, 7) | 1ULL << make_square(2, 7) | 1ULL << make_square(3, 7))) &&
            !Attacks::is_attacked(pos, make_square(2, 7), WHITE) &&
            !Attacks::is_attacked(pos, make_square(3, 7), WHITE)) {
            moves.push_back(Move(king_sq, make_square(2, 7), NO_PIECE, false, true));
        }
    }
}

uint64_t MoveGen::perft(const Position& pos, int depth) {
    if (depth == 0) return 1;
    
    std::vector<Move> moves;
    generate_legal(pos, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    Position temp_pos = pos;
    for (Move move : moves) {
        UndoInfo undo;
        temp_pos.make_move(move, undo);
        nodes += perft(temp_pos, depth - 1);
        temp_pos.unmake_move(move, undo);
    }
    
    return nodes;
}

} // namespace chess

