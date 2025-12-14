#include "position.h"
#include "../eval/evaluator.h"
#include "../movegen/attacks.h"
#include <sstream>
#include <random>
#include <cstring>

namespace chess {

Position::Position() : stm(WHITE), castlingRights(ANY_CASTLING), epSquare(SQ_NONE),
                       rule50Count(0), gamePly(0), zobristKey(0), evaluator(nullptr) {
    std::memset(board, 0, sizeof(board));
    std::memset(byColor, 0, sizeof(byColor));
    std::memset(byType, 0, sizeof(byType));
    kingSq[WHITE] = W_KING_START;
    kingSq[BLACK] = B_KING_START;
    init_zobrist();
}

void Position::init_zobrist() {
    std::mt19937_64 rng(12345); // Fixed seed for reproducibility
    
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 16; j++) {
            zobrist_piece[i][j] = rng();
        }
    }
    for (int i = 0; i < 16; i++) {
        zobrist_castling[i] = rng();
    }
    for (int i = 0; i < 8; i++) {
        zobrist_ep[i] = rng();
    }
    zobrist_side = rng();
}

void Position::remove_piece(Square sq) {
    Piece pc = board[sq];
    if (pc == NO_PIECE_PIECE) return;
    
    Color c = color_of(pc);
    PieceType pt = type_of(pc);
    
    Bitboard bb = 1ULL << sq;
    byColor[c] ^= bb;
    byType[pt] ^= bb;
    board[sq] = NO_PIECE_PIECE;
    
    if (pt == KING) {
        kingSq[c] = SQ_NONE;
    }
    
    zobristKey ^= zobrist_piece[sq][pc];
}

void Position::put_piece(Piece pc, Square sq) {
    if (pc == NO_PIECE_PIECE) return;
    
    Color c = color_of(pc);
    PieceType pt = type_of(pc);
    
    Bitboard bb = 1ULL << sq;
    byColor[c] |= bb;
    byType[pt] |= bb;
    board[sq] = pc;
    
    if (pt == KING) {
        kingSq[c] = sq;
    }
    
    zobristKey ^= zobrist_piece[sq][pc];
}

void Position::move_piece(Square from, Square to) {
    Piece pc = board[from];
    if (pc == NO_PIECE_PIECE) return;
    
    remove_piece(from);
    put_piece(pc, to);
}

void Position::update_castling_rights(Square sq) {
    // Remove castling rights if rook or king moves
    if (sq == W_KING_START || sq == W_ROOK_OO || sq == W_ROOK_OOO) {
        castlingRights &= ~WHITE_CASTLING;
    }
    if (sq == B_KING_START || sq == B_ROOK_OO || sq == B_ROOK_OOO) {
        castlingRights &= ~BLACK_CASTLING;
    }
    
    // Update zobrist
    uint64_t old_key = zobristKey;
    zobristKey ^= zobrist_castling[castlingRights];
}

bool Position::make_move(Move move, UndoInfo& undo) {
    if (!move.is_valid()) return false;
    
    Square from = move.from();
    Square to = move.to();
    Piece pc = board[from];
    
    if (pc == NO_PIECE_PIECE) return false;
    if (color_of(pc) != stm) return false;
    
    // Save undo information
    undo.move = move;
    undo.captured_piece = board[to];
    undo.captured_square = to;
    undo.castling_rights = castlingRights;
    undo.ep_square = epSquare;
    undo.rule50 = rule50Count;
    undo.zobrist_key = zobristKey;
    
    // Initialize NNUE delta
    undo.nnue_delta.moved_from = from;
    undo.nnue_delta.moved_to = to;
    undo.nnue_delta.moved_piece = pc;
    undo.nnue_delta.captured_square = SQ_NONE;
    undo.nnue_delta.captured_piece = NO_PIECE_PIECE;
    undo.nnue_delta.promo_square = SQ_NONE;
    undo.nnue_delta.promo_piece = NO_PIECE_PIECE;
    undo.nnue_delta.is_castling = false;
    undo.nnue_delta.is_en_passant = false;
    
    // Update rule50
    rule50Count++;
    if (type_of(pc) == PAWN || undo.captured_piece != NO_PIECE_PIECE) {
        rule50Count = 0;
    }
    
    // Handle captures
    if (undo.captured_piece != NO_PIECE_PIECE) {
        undo.nnue_delta.captured_square = to;
        undo.nnue_delta.captured_piece = undo.captured_piece;
        remove_piece(to);
    }
    
    // Handle en passant
    if (move.is_en_passant()) {
        Square ep_cap = stm == WHITE ? to - 8 : to + 8;
        undo.nnue_delta.is_en_passant = true;
        undo.nnue_delta.ep_captured_square = ep_cap;
        undo.nnue_delta.captured_piece = board[ep_cap];
        remove_piece(ep_cap);
    }
    
    // Handle castling
    if (move.is_castling()) {
        undo.nnue_delta.is_castling = true;
        if (to == make_square(6, rank_of(from))) { // O-O
            undo.nnue_delta.rook_from = stm == WHITE ? W_ROOK_OO : B_ROOK_OO;
            undo.nnue_delta.rook_to = stm == WHITE ? make_square(5, 0) : make_square(5, 7);
            move_piece(undo.nnue_delta.rook_from, undo.nnue_delta.rook_to);
        } else { // O-O-O
            undo.nnue_delta.rook_from = stm == WHITE ? W_ROOK_OOO : B_ROOK_OOO;
            undo.nnue_delta.rook_to = stm == WHITE ? make_square(3, 0) : make_square(3, 7);
            move_piece(undo.nnue_delta.rook_from, undo.nnue_delta.rook_to);
        }
    }
    
    // Move the piece
    move_piece(from, to);
    
    // Handle promotion
    if (move.is_promotion()) {
        PieceType promo_pt = move.promotion();
        remove_piece(to);
        put_piece(make_piece(stm, promo_pt), to);
        undo.nnue_delta.promo_square = to;
        undo.nnue_delta.promo_piece = make_piece(stm, promo_pt);
    }
    
    // Update castling rights
    update_castling_rights(from);
    if (undo.captured_piece != NO_PIECE_PIECE) {
        update_castling_rights(to);
    }
    
    // Update en passant square
    epSquare = SQ_NONE;
    if (type_of(pc) == PAWN && (rank_of(to) > rank_of(from) ? (rank_of(to) - rank_of(from)) : (rank_of(from) - rank_of(to))) == 2) {
        epSquare = stm == WHITE ? to - 8 : to + 8;
        zobristKey ^= zobrist_ep[file_of(epSquare)];
    }
    
    // Update zobrist
    zobristKey ^= zobrist_side;
    
    // Switch side
    stm = static_cast<Color>(1 - stm);
    gamePly++;
    
    // Notify evaluator
    if (evaluator) {
        evaluator->on_make_move(*this, move, undo);
    }
    
    return true;
}

void Position::unmake_move(Move move, const UndoInfo& undo) {
    // Switch side back
    stm = static_cast<Color>(1 - stm);
    gamePly--;
    
    // Restore zobrist
    zobristKey = undo.zobrist_key;
    
    // Restore position state
    castlingRights = undo.castling_rights;
    epSquare = undo.ep_square;
    rule50Count = undo.rule50;
    
    Square from = move.from();
    Square to = move.to();
    
    // Handle promotion
    if (move.is_promotion()) {
        remove_piece(to);
        put_piece(make_piece(stm, PAWN), to);
    }
    
    // Move piece back
    move_piece(to, from);
    
    // Handle castling
    if (move.is_castling()) {
        move_piece(undo.nnue_delta.rook_to, undo.nnue_delta.rook_from);
    }
    
    // Restore captured piece
    if (undo.captured_piece != NO_PIECE_PIECE) {
        if (move.is_en_passant()) {
            put_piece(undo.nnue_delta.captured_piece, undo.nnue_delta.ep_captured_square);
        } else {
            put_piece(undo.captured_piece, undo.captured_square);
        }
    }
    
    // Notify evaluator
    if (evaluator) {
        evaluator->on_unmake_move(*this, move, undo);
    }
}

bool Position::is_check() const {
    return Attacks::in_check(*this, stm);
}

bool Position::is_check(Color c) const {
    return Attacks::in_check(*this, c);
}

bool Position::is_legal(Move move) const {
    // This will be implemented in movegen module
    // For now, return true as placeholder
    return true;
}

bool Position::is_draw() const {
    return rule50Count >= 100; // Simplified - should check 3-fold repetition too
}

void Position::from_fen(const std::string& fen) {
    // Clear position
    std::memset(board, 0, sizeof(board));
    std::memset(byColor, 0, sizeof(byColor));
    std::memset(byType, 0, sizeof(byType));
    kingSq[WHITE] = SQ_NONE;
    kingSq[BLACK] = SQ_NONE;
    
    std::istringstream iss(fen);
    std::string token;
    
    // Piece placement
    iss >> token;
    Square sq = 56; // a8
    for (char c : token) {
        if (c == '/') {
            sq -= 16;
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            Piece pc = NO_PIECE_PIECE;
            Color col = (c >= 'A' && c <= 'Z') ? WHITE : BLACK;
            char lower = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            
            switch (lower) {
                case 'p': pc = make_piece(col, PAWN); break;
                case 'n': pc = make_piece(col, KNIGHT); break;
                case 'b': pc = make_piece(col, BISHOP); break;
                case 'r': pc = make_piece(col, ROOK); break;
                case 'q': pc = make_piece(col, QUEEN); break;
                case 'k': pc = make_piece(col, KING); break;
            }
            put_piece(pc, sq);
            sq++;
        }
    }
    
    // Side to move
    iss >> token;
    stm = (token == "w") ? WHITE : BLACK;
    
    // Castling rights
    iss >> token;
    castlingRights = 0;
    for (char c : token) {
        if (c == 'K') castlingRights |= WHITE_OO;
        if (c == 'Q') castlingRights |= WHITE_OOO;
        if (c == 'k') castlingRights |= BLACK_OO;
        if (c == 'q') castlingRights |= BLACK_OOO;
    }
    
    // En passant
    iss >> token;
    if (token == "-") {
        epSquare = SQ_NONE;
    } else {
        int file = token[0] - 'a';
        int rank = token[1] - '1';
        epSquare = make_square(file, rank);
    }
    
    // Rule 50 and game ply
    iss >> rule50Count;
    iss >> gamePly;
    
    // Recalculate zobrist key
    zobristKey = 0;
    for (int i = 0; i < 64; i++) {
        if (board[i] != NO_PIECE_PIECE) {
            zobristKey ^= zobrist_piece[i][board[i]];
        }
    }
    zobristKey ^= zobrist_castling[castlingRights];
    if (epSquare != SQ_NONE) {
        zobristKey ^= zobrist_ep[file_of(epSquare)];
    }
    if (stm == BLACK) {
        zobristKey ^= zobrist_side;
    }
}

std::string Position::to_fen() const {
    std::ostringstream oss;
    
    // Piece placement
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Square sq = make_square(file, rank);
            Piece pc = board[sq];
            if (pc == NO_PIECE_PIECE) {
                empty++;
            } else {
                if (empty > 0) {
                    oss << empty;
                    empty = 0;
                }
                char c = " PNBRQK  pnbrqk"[pc];
                oss << c;
            }
        }
        if (empty > 0) oss << empty;
        if (rank > 0) oss << '/';
    }
    
    oss << ' ' << (stm == WHITE ? 'w' : 'b') << ' ';
    
    // Castling
    if (castlingRights == 0) {
        oss << '-';
    } else {
        if (castlingRights & WHITE_OO) oss << 'K';
        if (castlingRights & WHITE_OOO) oss << 'Q';
        if (castlingRights & BLACK_OO) oss << 'k';
        if (castlingRights & BLACK_OOO) oss << 'q';
    }
    
    oss << ' ';
    
    // En passant
    if (epSquare == SQ_NONE) {
        oss << '-';
    } else {
        oss << char('a' + file_of(epSquare)) << char('1' + rank_of(epSquare));
    }
    
    oss << ' ' << rule50Count << ' ' << gamePly;
    
    return oss.str();
}

Position::Position(const std::string& fen) : stm(WHITE), castlingRights(ANY_CASTLING), epSquare(SQ_NONE),
                                             rule50Count(0), gamePly(0), zobristKey(0), evaluator(nullptr) {
    std::memset(board, 0, sizeof(board));
    std::memset(byColor, 0, sizeof(byColor));
    std::memset(byType, 0, sizeof(byType));
    kingSq[WHITE] = W_KING_START;
    kingSq[BLACK] = B_KING_START;
    init_zobrist();
    from_fen(fen);
}

} // namespace chess

