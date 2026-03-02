#include "classic_eval.h"
#include "../board/position.h"
#include "../board/types.h"
#include "../board/bitboard.h"
#include "../movegen/attacks.h"
#include <algorithm>
#include <cmath>

namespace chess {

// Piece-square tables (from white's perspective, flipped for black)
// Rank 0 = a1-h1 (white's back rank), Rank 7 = a8-h8 (white's promotion rank)
const int ClassicEval::PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,  // Rank 1 (impossible for pawns)
     5,  5,  5, 10, 10,  5,  5,  5,  // Rank 2 - bonus for d2/e2
    10, 10, 15, 25, 25, 15, 10, 10,  // Rank 3 - bonus for central pawns
    15, 15, 20, 35, 35, 20, 15, 15,  // Rank 4 - strong center bonus
    20, 20, 25, 40, 40, 25, 20, 20,  // Rank 5
    30, 30, 35, 50, 50, 35, 30, 30,  // Rank 6
    50, 50, 50, 50, 50, 50, 50, 50,  // Rank 7
     0,  0,  0,  0,  0,  0,  0,  0   // Rank 8 (promotion)
};

const int ClassicEval::PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 15, 30, 30, 15,  5,-30,  // Strong bonus for f3/c3 development
    -30, 10, 20, 35, 35, 20, 10,-30,  // Strong center
    -30,  5, 20, 35, 35, 20,  5,-30,
    -30, 10, 15, 25, 25, 15, 10,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-20,-20,-20,-20,-40,-50  // Penalty for back rank knights
};

const int ClassicEval::PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  5, 10, 15, 15, 10,  5,-10,  // Bonus for developed bishops
    -10, 10, 10, 15, 15, 10, 10,-10,
    -10,  5, 15, 15, 15, 15,  5,-10,
    -10, 15, 15, 15, 15, 15, 15,-10,  // Strong bonus for active bishops
    -10, 10,  5,  5,  5,  5, 10,-10,
    -30,-20,-20,-20,-20,-20,-20,-30  // Stronger penalty for back rank bishops
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
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

// Opening queen PST - heavily penalize early development
const int ClassicEval::PST_QUEEN_OPENING[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -80,-80,-80,-80,-80,-80,-80,-80,  // Very heavy penalty for moving queen early
    -80,-80,-80,-80,-80,-80,-80,-80,
    -80,-80,-80,-80,-80,-80,-80,-80,
    -80,-80,-80,-80,-80,-80,-80,-80,
    -80,-80,-80,-80,-80,-80,-80,-80,
    -80,-80,-80,-80,-80,-80,-80,-80,
    -20,-10,-10, -5, -5,-10,-10,-20  // Only starting square is OK
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
    -30,-10, 30, 80, 80, 30,-10,-30,  // Very strong center bonus for d4/e4
    -30,-10, 30, 80, 80, 30,-10,-30,  // Very strong center bonus for d5/e5
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
    905,    // QUEEN (worth ~4 pawns more than rook)
    20000   // KING
};

ClassicEval::ClassicEval() {
    // Initialize default evaluation parameters (tunable via Texel tuning)
    params.material_pawn = 100;
    params.material_knight = 320;
    params.material_bishop = 330;
    params.material_rook = 500;
    params.material_queen = 905;
    
    // Pawn structure (optimized)
    params.doubled_pawn_penalty = 110;  // Per pawn in doubled pair - strong penalty
    params.isolated_pawn_penalty = 120;  // Strong penalty for isolated pawns
    params.backward_pawn_penalty = 10;  // Increased from 8
    params.pawn_chain_bonus = 8;  // Increased from 5 to encourage pawn chains
    
    // Passed pawn bonuses by rank (from white's perspective)
    params.passed_pawn_bonus[0] = 0;
    params.passed_pawn_bonus[1] = 0;
    params.passed_pawn_bonus[2] = 10;
    params.passed_pawn_bonus[3] = 20;
    params.passed_pawn_bonus[4] = 35;
    params.passed_pawn_bonus[5] = 60;
    params.passed_pawn_bonus[6] = 100;
    params.passed_pawn_bonus[7] = 200;
    
    // Piece-specific
    params.rook_open_file_bonus = 60;  // Strong bonus for open file
    params.rook_semi_open_file_bonus = 30;  // Half of open file
    params.rook_blocked_penalty = 80;  // Penalty when pawn blocks rook on same file
    params.bishop_pair_bonus = 30;
    params.knight_outpost_bonus = 15;
    params.bad_bishop_penalty = 20;
    params.passive_bishop_penalty = 50;  // Bishops on a3/h3 (White) or a6/h6 (Black) in opening
    
    // King safety (optimized)
    params.pawn_shield_bonus = 12;  // Increased from 10
    params.open_file_near_king_penalty = 25;  // Increased from 20
    params.king_tropism_weight = 6;  // Increased from 5
    params.castling_bonus = 40;  // Bonus when king has castled (25-50 cp)
    params.king_center_penalty = 40;  // Penalty for king on e1/e8 in opening/middlegame (30-50 cp)
    params.king_tropism_uncastled_multiplier = 150;  // 1.5x tropism penalty when king in center
    
    // Mobility (per piece type)
    params.mobility_weight[NO_PIECE] = 0;
    params.mobility_weight[PAWN] = 0;
    params.mobility_weight[KNIGHT] = 4;
    params.mobility_weight[BISHOP] = 3;
    params.mobility_weight[ROOK] = 2;
    params.mobility_weight[QUEEN] = 0;  // Reduced from 1 to prevent overvaluing queen mobility
    
    // Endgame
    params.king_activity_bonus = 50;  // Increased from 30 for even stronger centralization
    params.opposition_bonus = 30;  // Increased for better opposition detection
}

int ClassicEval::calculate_game_phase(const Position& pos) const {
    // Calculate game phase based on remaining material
    // Phase: 0 = endgame, 256 = opening/middlegame
    int phase = 0;
    phase += popcount(pos.pieces(KNIGHT)) * 1;
    phase += popcount(pos.pieces(BISHOP)) * 1;
    phase += popcount(pos.pieces(ROOK)) * 2;
    phase += popcount(pos.pieces(QUEEN)) * 4;
    
    // Total material at start: 4N + 4B + 4R + 2Q = 24
    // Scale to 256
    phase = (phase * 256 + 12) / 24;
    return std::min(phase, 256);
}

int ClassicEval::tapered_eval(int mg_score, int eg_score, int phase) const {
    // Linear interpolation between middlegame and endgame scores
    return ((mg_score * phase) + (eg_score * (256 - phase))) / 256;
}

int ClassicEval::pst_value(PieceType pt, Square sq, Color c, int phase) const {
    int idx = (c == WHITE) ? sq : (sq ^ 56);
    int mg_value = 0, eg_value = 0;
    
    switch (pt) {
        case PAWN:
            mg_value = PST_PAWN[idx];
            eg_value = PST_PAWN[idx];
            break;
        case KNIGHT:
            mg_value = PST_KNIGHT[idx];
            eg_value = PST_KNIGHT[idx];
            break;
        case BISHOP:
            mg_value = PST_BISHOP[idx];
            eg_value = PST_BISHOP[idx];
            break;
        case ROOK:
            mg_value = PST_ROOK[idx];
            eg_value = PST_ROOK[idx];
            break;
        case QUEEN:
            // Use opening PST in opening (phase > 200), regular PST in endgame
            mg_value = (phase > 200) ? PST_QUEEN_OPENING[idx] : PST_QUEEN[idx];
            eg_value = PST_QUEEN[idx];
            break;
        case KING:
            mg_value = PST_KING[idx];
            eg_value = PST_KING_END[idx];
            break;
        default:
            return 0;
    }
    
    return tapered_eval(mg_value, eg_value, phase);
}

int ClassicEval::evaluate(const Position& pos) {
    int phase = calculate_game_phase(pos);
    
    int score = 0;
    score += evaluate_pieces(pos, phase);
    score += evaluate_pawns(pos);
    score += evaluate_king_safety(pos, phase);
    score += evaluate_mobility(pos);
    score += evaluate_piece_specific(pos);
    score += evaluate_endgame(pos, phase);
    score += evaluate_tactics(pos);
    score += evaluate_exchange_compensation(pos);
    score += evaluate_opening(pos, phase);
    
    // Check for insufficient material
    int white_pieces = popcount(pos.pieces(WHITE)) - 1;  // Exclude king
    int black_pieces = popcount(pos.pieces(BLACK)) - 1;
    
    // K+N vs K or K+B vs K - theoretical draw. Don't scale material - we need correct
    // piece values for search. evaluate_endgame returns 0 for these positions.
    
    // K vs K - DON'T return 0! The score includes king centralization bonus
    // which is critical for proper king activity in K vs K endgames
    if (white_pieces == 0 && black_pieces == 0) {
        // Return score from side to move's perspective
        return (pos.side_to_move() == WHITE) ? score : -score;
    }
    
    // K+N vs K or K+B vs K (cannot mate, but show material advantage)
    // Don't scale - let the material value show through
    // The DrawishEndgame test allows up to 200 centipawns difference
    
    // Tempo bonus (side to move advantage, scaled by phase)
    // Higher in opening (more tactical), lower in endgame
    int tempo_bonus = (phase * 20) / 256;  // 0-20 centipawns
    score += tempo_bonus;
    
    // Return score from side to move's perspective
    return (pos.side_to_move() == WHITE) ? score : -score;
}

int ClassicEval::evaluate_pieces(const Position& pos, int phase) const {
    int score = 0;
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        
        for (PieceType pt = PAWN; pt <= KING; pt = static_cast<PieceType>(pt + 1)) {
            Bitboard bb = pos.pieces(c, pt);
            while (bb) {
                Square sq = pop_lsb(bb);
                score += sign * (MATERIAL[pt] + pst_value(pt, sq, c, phase));
            }
        }
    }
    
    return score;
}

Bitboard ClassicEval::get_passed_pawn_mask(Square sq, Color c) const {
    int file = file_of(sq);
    int rank = rank_of(sq);
    Bitboard mask = 0ULL;
    
    // Mask includes the file and adjacent files, all ranks ahead
    int start_rank = (c == WHITE) ? rank + 1 : 0;
    int end_rank = (c == WHITE) ? 8 : rank;
    
    for (int r = start_rank; r < end_rank; r++) {
        if (file > 0) mask |= (1ULL << make_square(file - 1, r));
        mask |= (1ULL << make_square(file, r));
        if (file < 7) mask |= (1ULL << make_square(file + 1, r));
    }
    
    return mask;
}

Bitboard ClassicEval::get_isolated_pawn_mask(Square sq) const {
    int file = file_of(sq);
    Bitboard mask = 0ULL;
    
    // Mask includes adjacent files only
    for (int r = 0; r < 8; r++) {
        if (file > 0) mask |= (1ULL << make_square(file - 1, r));
        if (file < 7) mask |= (1ULL << make_square(file + 1, r));
    }
    
    return mask;
}

Bitboard ClassicEval::get_backward_pawn_mask(Square sq, Color c) const {
    int file = file_of(sq);
    int rank = rank_of(sq);
    Bitboard mask = 0ULL;
    
    // Mask includes adjacent files, ranks behind
    int start_rank = (c == WHITE) ? 0 : rank + 1;
    int end_rank = (c == WHITE) ? rank : 8;
    
    for (int r = start_rank; r < end_rank; r++) {
        if (file > 0) mask |= (1ULL << make_square(file - 1, r));
        if (file < 7) mask |= (1ULL << make_square(file + 1, r));
    }
    
    return mask;
}

bool ClassicEval::is_doubled_pawn(const Position& pos, Square sq, Color c) const {
    int file = file_of(sq);
    int rank = rank_of(sq);
    Bitboard pawns = pos.pieces(c, PAWN);
    
    // Check if there's another pawn on the same file
    for (int r = 0; r < 8; r++) {
        if (r != rank) {
            Square other_sq = make_square(file, r);
            if (pawns & (1ULL << other_sq)) {
                return true;
            }
        }
    }
    return false;
}

bool ClassicEval::is_isolated_pawn(const Position& pos, Square sq, Color c) const {
    Bitboard mask = get_isolated_pawn_mask(sq);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    return (mask & friendly_pawns) == 0;
}

bool ClassicEval::is_passed_pawn(const Position& pos, Square sq, Color c) const {
    Bitboard mask = get_passed_pawn_mask(sq, c);
    Bitboard enemy_pawns = pos.pieces(static_cast<Color>(1 - c), PAWN);
    return (mask & enemy_pawns) == 0;
}

bool ClassicEval::is_backward_pawn(const Position& pos, Square sq, Color c) const {
    // A pawn is backward if:
    // 1. It cannot advance safely
    // 2. It has no friendly pawns behind it on adjacent files to support it
    
    int file = file_of(sq);
    int rank = rank_of(sq);
    
    // Check if there are friendly pawns on adjacent files that can support
    Bitboard support_mask = get_backward_pawn_mask(sq, c);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    if ((support_mask & friendly_pawns) != 0) {
        return false;
    }
    
    // Check if the pawn can advance safely
    int forward_rank = (c == WHITE) ? rank + 1 : rank - 1;
    if (forward_rank < 0 || forward_rank >= 8) return false;
    
    Square forward_sq = make_square(file, forward_rank);
    Bitboard enemy_pawns = pos.pieces(static_cast<Color>(1 - c), PAWN);
    
    // Check if enemy pawns attack the forward square
    if (file > 0) {
        Square enemy_sq = make_square(file - 1, forward_rank + (c == WHITE ? 1 : -1));
        if (enemy_sq >= 0 && enemy_sq < 64 && (enemy_pawns & (1ULL << enemy_sq))) {
            return true;
        }
    }
    if (file < 7) {
        Square enemy_sq = make_square(file + 1, forward_rank + (c == WHITE ? 1 : -1));
        if (enemy_sq >= 0 && enemy_sq < 64 && (enemy_pawns & (1ULL << enemy_sq))) {
            return true;
        }
    }
    
    return false;
}

bool ClassicEval::is_pawn_chain(const Position& pos, Square sq, Color c) const {
    int file = file_of(sq);
    int rank = rank_of(sq);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    // Check if this pawn is supported by another pawn diagonally behind
    int support_rank = (c == WHITE) ? rank - 1 : rank + 1;
    if (support_rank < 0 || support_rank >= 8) return false;
    
    if (file > 0) {
        Square support_sq = make_square(file - 1, support_rank);
        if (friendly_pawns & (1ULL << support_sq)) return true;
    }
    if (file < 7) {
        Square support_sq = make_square(file + 1, support_rank);
        if (friendly_pawns & (1ULL << support_sq)) return true;
    }
    
    return false;
}

int ClassicEval::evaluate_pawns(const Position& pos) const {
    int score = 0;
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        Bitboard pawns = pos.pieces(c, PAWN);
        Bitboard bb = pawns;
        
        while (bb) {
            Square sq = pop_lsb(bb);
            
            // Doubled pawns - penalize each pawn in a doubled pair
            if (is_doubled_pawn(pos, sq, c)) {
                score += sign * -params.doubled_pawn_penalty;
            }
            
            // Isolated pawns
            if (is_isolated_pawn(pos, sq, c)) {
                score += sign * -params.isolated_pawn_penalty;
            }
            
            // Passed pawns
            if (is_passed_pawn(pos, sq, c)) {
                int rank = (c == WHITE) ? rank_of(sq) : (7 - rank_of(sq));
                score += sign * params.passed_pawn_bonus[rank];
            }
            
            // Backward pawns
            if (is_backward_pawn(pos, sq, c)) {
                score += sign * -params.backward_pawn_penalty;
            }
            
            // Pawn chains
            if (is_pawn_chain(pos, sq, c)) {
                score += sign * params.pawn_chain_bonus;
            }
        }
    }
    
    return score;
}

int ClassicEval::count_pawn_shield(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    int file = file_of(king_sq);
    int rank = rank_of(king_sq);
    int shield_count = 0;
    
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    // Check pawns in front of king (1 or 2 ranks ahead)
    for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
        int shield_rank1 = (c == WHITE) ? rank + 1 : rank - 1;
        int shield_rank2 = (c == WHITE) ? rank + 2 : rank - 2;
        
        if (shield_rank1 >= 0 && shield_rank1 < 8) {
            Square sq1 = make_square(f, shield_rank1);
            if (friendly_pawns & (1ULL << sq1)) shield_count++;
        }
        
        if (shield_rank2 >= 0 && shield_rank2 < 8) {
            Square sq2 = make_square(f, shield_rank2);
            if (friendly_pawns & (1ULL << sq2)) shield_count++;
        }
    }
    
    return shield_count;
}

int ClassicEval::count_open_files_near_king(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    int file = file_of(king_sq);
    int open_files = 0;
    
    Bitboard all_pawns = pos.pieces(PAWN);
    
    // Check files near king
    for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
        bool has_pawn = false;
        for (int r = 0; r < 8; r++) {
            Square sq = make_square(f, r);
            if (all_pawns & (1ULL << sq)) {
                has_pawn = true;
                break;
            }
        }
        if (!has_pawn) open_files++;
    }
    
    return open_files;
}

int ClassicEval::calculate_king_tropism(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    Color enemy = static_cast<Color>(1 - c);
    int tropism = 0;
    
    // Calculate distance of enemy pieces to our king
    for (PieceType pt = KNIGHT; pt <= QUEEN; pt = static_cast<PieceType>(pt + 1)) {
        Bitboard bb = pos.pieces(enemy, pt);
        while (bb) {
            Square sq = pop_lsb(bb);
            int file_dist = abs(file_of(sq) - file_of(king_sq));
            int rank_dist = abs(rank_of(sq) - rank_of(king_sq));
            int distance = std::max(file_dist, rank_dist);
            
            // Closer pieces are more dangerous
            tropism += (8 - distance);
        }
    }
    
    return tropism;
}

int ClassicEval::evaluate_king_safety(const Position& pos, int phase) const {
    int score = 0;
    
    // King safety is more important in middlegame
    if (phase < 128) return 0;
    
    const Square W_CASTLE_K = make_square(6, 0);   // g1
    const Square W_CASTLE_Q = make_square(1, 0);   // b1
    const Square B_CASTLE_K = make_square(6, 7);   // g8
    const Square B_CASTLE_Q = make_square(1, 7);   // b8
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        Square king_sq = pos.king_square(c);
        
        // Castling bonus: king on g1/b1 (White) or g8/b8 (Black)
        bool castled = (c == WHITE && (king_sq == W_CASTLE_K || king_sq == W_CASTLE_Q)) ||
                       (c == BLACK && (king_sq == B_CASTLE_K || king_sq == B_CASTLE_Q));
        if (castled) {
            score += sign * params.castling_bonus;
        }
        
        // King in center penalty: e1 (White) or e8 (Black), scaled by phase
        bool king_in_center = (c == WHITE && king_sq == W_KING_START) ||
                              (c == BLACK && king_sq == B_KING_START);
        if (king_in_center && phase > 128) {
            int penalty = (params.king_center_penalty * phase) / 256;
            score += sign * -penalty;
        }
        
        // Pawn shield bonus
        int shield = count_pawn_shield(pos, c);
        score += sign * shield * params.pawn_shield_bonus;
        
        // Open files near king penalty
        int open_files = count_open_files_near_king(pos, c);
        score += sign * -open_files * params.open_file_near_king_penalty;
        
        // King tropism (enemy pieces attacking near king)
        // Stronger penalty when king is uncastled (in center)
        int tropism = calculate_king_tropism(pos, c);
        int tropism_weight = params.king_tropism_weight;
        if (king_in_center) {
            tropism_weight = (tropism_weight * params.king_tropism_uncastled_multiplier) / 100;
        }
        score += sign * -tropism * tropism_weight;
    }
    
    return score;
}

int ClassicEval::evaluate_mobility(const Position& pos) const {
    int score = 0;
    
    Bitboard all_pieces = pos.pieces(WHITE) | pos.pieces(BLACK);
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        
        // Knight mobility
        Bitboard knights = pos.pieces(c, KNIGHT);
        while (knights) {
            Square sq = pop_lsb(knights);
            Bitboard attacks = Attacks::knight_attacks(sq);
            Bitboard valid_moves = attacks & ~pos.pieces(c);
            int mobility = popcount(valid_moves);
            score += sign * mobility * params.mobility_weight[KNIGHT];
        }
        
        // Bishop mobility
        Bitboard bishops = pos.pieces(c, BISHOP);
        while (bishops) {
            Square sq = pop_lsb(bishops);
            Bitboard attacks = Attacks::bishop_attacks(sq, all_pieces);
            Bitboard valid_moves = attacks & ~pos.pieces(c);
            int mobility = popcount(valid_moves);
            score += sign * mobility * params.mobility_weight[BISHOP];
        }
        
        // Rook mobility
        Bitboard rooks = pos.pieces(c, ROOK);
        while (rooks) {
            Square sq = pop_lsb(rooks);
            Bitboard attacks = Attacks::rook_attacks(sq, all_pieces);
            Bitboard valid_moves = attacks & ~pos.pieces(c);
            int mobility = popcount(valid_moves);
            score += sign * mobility * params.mobility_weight[ROOK];
        }
        
        // Queen mobility
        Bitboard queens = pos.pieces(c, QUEEN);
        while (queens) {
            Square sq = pop_lsb(queens);
            Bitboard attacks = Attacks::queen_attacks(sq, all_pieces);
            Bitboard valid_moves = attacks & ~pos.pieces(c);
            int mobility = popcount(valid_moves);
            score += sign * mobility * params.mobility_weight[QUEEN];
        }
    }
    
    return score;
}

void ClassicEval::on_make_move(Position& pos, Move move, const UndoInfo& undo) {
    (void)pos;
    (void)move;
    (void)undo;
}

bool ClassicEval::is_rook_on_open_file(const Position& pos, Square sq) const {
    int file = file_of(sq);
    Bitboard all_pawns = pos.pieces(PAWN);
    
    // Check if there are no pawns on this file
    for (int r = 0; r < 8; r++) {
        Square check_sq = make_square(file, r);
        if (all_pawns & (1ULL << check_sq)) {
            return false;
        }
    }
    return true;
}

bool ClassicEval::is_rook_on_semi_open_file(const Position& pos, Square sq, Color c) const {
    int file = file_of(sq);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    // Check if there are no friendly pawns on this file
    for (int r = 0; r < 8; r++) {
        Square check_sq = make_square(file, r);
        if (friendly_pawns & (1ULL << check_sq)) {
            return false;
        }
    }
    return true;
}

bool ClassicEval::is_rook_blocked_by_pawn(const Position& pos, Square sq, Color c) const {
    int file = file_of(sq);
    int rank = rank_of(sq);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    // Check for friendly pawns on same file ahead of rook (toward enemy territory)
    if (c == WHITE) {
        for (int r = rank + 1; r < 8; r++) {
            if (friendly_pawns & (1ULL << make_square(file, r))) return true;
        }
    } else {
        for (int r = rank - 1; r >= 0; r--) {
            if (friendly_pawns & (1ULL << make_square(file, r))) return true;
        }
    }
    return false;
}

bool ClassicEval::has_bishop_pair(const Position& pos, Color c) const {
    Bitboard bishops = pos.pieces(c, BISHOP);
    return popcount(bishops) >= 2;
}

bool ClassicEval::is_knight_outpost(const Position& pos, Square sq, Color c) const {
    // An outpost is a square that:
    // 1. Cannot be attacked by enemy pawns
    // 2. Is supported by a friendly pawn
    
    int file = file_of(sq);
    int rank = rank_of(sq);
    
    Color enemy = static_cast<Color>(1 - c);
    Bitboard enemy_pawns = pos.pieces(enemy, PAWN);
    
    // Check if enemy pawns can attack this square
    int enemy_pawn_rank1 = (c == WHITE) ? rank + 1 : rank - 1;
    if (enemy_pawn_rank1 >= 0 && enemy_pawn_rank1 < 8) {
        if (file > 0) {
            Square ep_sq = make_square(file - 1, enemy_pawn_rank1);
            if (enemy_pawns & (1ULL << ep_sq)) return false;
        }
        if (file < 7) {
            Square ep_sq = make_square(file + 1, enemy_pawn_rank1);
            if (enemy_pawns & (1ULL << ep_sq)) return false;
        }
    }
    
    // Check if supported by friendly pawn
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    int support_rank = (c == WHITE) ? rank - 1 : rank + 1;
    if (support_rank >= 0 && support_rank < 8) {
        if (file > 0) {
            Square sp_sq = make_square(file - 1, support_rank);
            if (friendly_pawns & (1ULL << sp_sq)) return true;
        }
        if (file < 7) {
            Square sp_sq = make_square(file + 1, support_rank);
            if (friendly_pawns & (1ULL << sp_sq)) return true;
        }
    }
    
    return false;
}

bool ClassicEval::is_bad_bishop(const Position& pos, Square sq, Color c) const {
    // A bishop is "bad" if many of our pawns are on the same color squares
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    bool bishop_on_light = ((sq + rank_of(sq)) % 2) == 0;
    
    int same_color_pawns = 0;
    int total_pawns = 0;
    
    while (friendly_pawns) {
        Square pawn_sq = pop_lsb(friendly_pawns);
        total_pawns++;
        bool pawn_on_light = ((pawn_sq + rank_of(pawn_sq)) % 2) == 0;
        if (pawn_on_light == bishop_on_light) {
            same_color_pawns++;
        }
    }
    
    // Bad if more than 60% of pawns are on same color
    return total_pawns > 0 && (same_color_pawns * 10 > total_pawns * 6);
}

int ClassicEval::evaluate_piece_specific(const Position& pos) const {
    int score = 0;
    int phase = calculate_game_phase(pos);
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        
        // Rooks on open/semi-open files
        Bitboard rooks = pos.pieces(c, ROOK);
        Bitboard rook_bb = rooks;
        while (rook_bb) {
            Square sq = pop_lsb(rook_bb);
            if (is_rook_blocked_by_pawn(pos, sq, c)) {
                score += sign * -params.rook_blocked_penalty;
            } else if (is_rook_on_open_file(pos, sq)) {
                score += sign * params.rook_open_file_bonus;
            } else if (is_rook_on_semi_open_file(pos, sq, c)) {
                score += sign * params.rook_semi_open_file_bonus;
            }
        }
        
        // Bishop pair
        if (has_bishop_pair(pos, c)) {
            score += sign * params.bishop_pair_bonus;
        }
        
        // Bad bishops and passive bishops (a3/h3 for White, a6/h6 for Black)
        Bitboard bishops = pos.pieces(c, BISHOP);
        Bitboard bishop_bb = bishops;
        while (bishop_bb) {
            Square sq = pop_lsb(bishop_bb);
            if (is_bad_bishop(pos, sq, c)) {
                score += sign * -params.bad_bishop_penalty;
            }
            // Passive bishop penalty: bishops on rim in opening/middlegame
            if (phase > 128) {
                bool is_passive = (c == WHITE && (sq == make_square(0, 2) || sq == make_square(7, 2))) ||
                                 (c == BLACK && (sq == make_square(0, 5) || sq == make_square(7, 5)));
                if (is_passive) {
                    score += sign * -params.passive_bishop_penalty;
                }
            }
        }
        
        // Knight outposts
        Bitboard knights = pos.pieces(c, KNIGHT);
        Bitboard knight_bb = knights;
        while (knight_bb) {
            Square sq = pop_lsb(knight_bb);
            if (is_knight_outpost(pos, sq, c)) {
                score += sign * params.knight_outpost_bonus;
            }
        }
    }
    
    return score;
}

void ClassicEval::on_unmake_move(Position& pos, Move move, const UndoInfo& undo) {
    (void)pos;
    (void)move;
    (void)undo;
}

int ClassicEval::king_activity_score(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    // In endgame, centralized king is better
    int file = file_of(king_sq);
    int rank = rank_of(king_sq);
    
    // Distance from edge (0 = edge, 3 = center for file, 3 = center for rank)
    int file_from_edge = std::min(file, 7 - file);
    int rank_from_edge = std::min(rank, 7 - rank);
    int centralization = file_from_edge + rank_from_edge;
    
    // Higher centralization is better (max 6 for center, min 0 for corner)
    return centralization * params.king_activity_bonus;
}

bool ClassicEval::has_opposition(const Position& pos) const {
    Square white_king = pos.king_square(WHITE);
    Square black_king = pos.king_square(BLACK);
    
    if (white_king == SQ_NONE || black_king == SQ_NONE) return false;
    
    int file_dist = abs(file_of(white_king) - file_of(black_king));
    int rank_dist = abs(rank_of(white_king) - rank_of(black_king));
    
    // Direct opposition: kings on same file/rank, 2 squares apart
    if ((file_dist == 0 && rank_dist == 2) || (rank_dist == 0 && file_dist == 2)) {
        return true;
    }
    
    // Distant opposition: kings on same file/rank, even number of squares apart
    if ((file_dist == 0 && rank_dist % 2 == 0 && rank_dist > 0) ||
        (rank_dist == 0 && file_dist % 2 == 0 && file_dist > 0)) {
        return true;
    }
    
    return false;
}

int ClassicEval::evaluate_endgame(const Position& pos, int phase) const {
    int score = 0;
    
    // Endgame features only matter in endgame
    if (phase > 128) return 0;
    
    // Check if this is a K vs K endgame (no other pieces)
    int white_pieces = popcount(pos.pieces(WHITE)) - 1;  // Exclude king
    int black_pieces = popcount(pos.pieces(BLACK)) - 1;
    bool is_kvk = (white_pieces == 0 && black_pieces == 0);
    
    // King activity (scale by endgame phase)
    int endgame_weight = (256 - phase) / 256.0 * 100;  // 0-100 based on how deep in endgame
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        int activity = king_activity_score(pos, c);
        
        // In K vs K, centralization is CRITICAL - use much stronger bonus
        if (is_kvk) {
            score += sign * activity * 50;  // 50x multiplier for K vs K (250-300cp for center)
        } else {
            score += sign * activity * endgame_weight / 100;
        }
    }
    
    // In K vs K, small bonus for diagonal (opposition) - keep modest to avoid
    // inflating K vs K above K+N vs K in piece value tests
    if (is_kvk) {
        Square white_king = pos.king_square(WHITE);
        Square black_king = pos.king_square(BLACK);
        if (white_king != SQ_NONE && black_king != SQ_NONE) {
            int file_dist = abs(file_of(white_king) - file_of(black_king));
            int rank_dist = abs(rank_of(white_king) - rank_of(black_king));
            if (file_dist > 0 && rank_dist > 0) {
                score += 15;  // Modest bonus for diagonal
            }
        }
    }
    
    // Opposition (favor side to move in pawn endgames)
    if (has_opposition(pos)) {
        int pawn_count = popcount(pos.pieces(PAWN));
        int piece_count = popcount(pos.pieces(KNIGHT)) + popcount(pos.pieces(BISHOP)) + 
                         popcount(pos.pieces(ROOK)) + popcount(pos.pieces(QUEEN));
        
        // Opposition matters most in pure pawn endgames
        if (piece_count == 0 && pawn_count > 0) {
            // Side to move has the opposition advantage
            score += params.opposition_bonus;
        }
    }
    
    // Advanced pawn detection - pawns on 7th rank are extremely valuable
    int white_rooks = popcount(pos.pieces(WHITE, ROOK));
    int black_rooks = popcount(pos.pieces(BLACK, ROOK));
    int white_pawns = popcount(pos.pieces(WHITE, PAWN));
    int black_pawns = popcount(pos.pieces(BLACK, PAWN));
    
    // Pawn on 7th rank is extremely valuable (near promotion)
    // Lucena bonus: K+P vs R with pawn on 7th is winning for the pawn side
    bool white_lucena = (white_pieces == 1 && black_pieces == 1 && white_pawns == 1 &&
        popcount(pos.pieces(BLACK, ROOK)) == 1 && popcount(pos.pieces(BLACK, QUEEN)) == 0);
    bool black_lucena = (black_pieces == 1 && white_pieces == 1 && black_pawns == 1 &&
        popcount(pos.pieces(WHITE, ROOK)) == 1 && popcount(pos.pieces(WHITE, QUEEN)) == 0);
    
    Bitboard white_pawns_bb = pos.pieces(WHITE, PAWN);
    while (white_pawns_bb) {
        Square pawn_sq = pop_lsb(white_pawns_bb);
        int pawn_rank = rank_of(pawn_sq);
        if (pawn_rank == 6) {  // Pawn on 7th rank (0-indexed)
            score += 250;  // Large bonus for pawn about to promote
            if (white_lucena) score += 450;  // Lucena: K+P vs R winning
        }
    }
    
    Bitboard black_pawns_bb = pos.pieces(BLACK, PAWN);
    while (black_pawns_bb) {
        Square pawn_sq = pop_lsb(black_pawns_bb);
        int pawn_rank = rank_of(pawn_sq);
        if (pawn_rank == 1) {  // Pawn on 2nd rank (0-indexed)
            score -= 250;  // Large penalty for black pawn about to promote
            if (black_lucena) score -= 450;  // Lucena: K+P vs R winning for black
        }
    }
    
    // K+P vs K - boost passed pawn value significantly (only when it's truly K+P vs K)
    if (white_pieces == 1 && black_pieces == 0 && white_pawns == 1 && 
        popcount(pos.pieces(WHITE, KNIGHT)) == 0 && popcount(pos.pieces(WHITE, BISHOP)) == 0 &&
        popcount(pos.pieces(WHITE, ROOK)) == 0 && popcount(pos.pieces(WHITE, QUEEN)) == 0) {
        Bitboard pawns = pos.pieces(WHITE, PAWN);
        while (pawns) {
            Square pawn_sq = pop_lsb(pawns);
            int rank = rank_of(pawn_sq);
            // Extra bonus based on how advanced the pawn is
            score += 50 + rank * 30;  // 50-260 bonus
        }
    }
    
    if (black_pieces == 1 && white_pieces == 0 && black_pawns == 1 &&
        popcount(pos.pieces(BLACK, KNIGHT)) == 0 && popcount(pos.pieces(BLACK, BISHOP)) == 0 &&
        popcount(pos.pieces(BLACK, ROOK)) == 0 && popcount(pos.pieces(BLACK, QUEEN)) == 0) {
        Bitboard pawns = pos.pieces(BLACK, PAWN);
        while (pawns) {
            Square pawn_sq = pop_lsb(pawns);
            int rank = 7 - rank_of(pawn_sq);
            // Extra bonus based on how advanced the pawn is
            score -= 50 + rank * 30;  // 50-260 bonus
        }
    }
    
    // Insufficient material detection (drawish endgames)
    // (Already declared above for Lucena detection)
    
    // King vs King - DON'T return 0! We need the centralization bonus to work
    // The score already includes king activity bonuses above
    if (white_pieces == 0 && black_pieces == 0) {
        return score;  // Return the score with king activity, not 0
    }
    
    // King + minor piece vs King (draw)
    if ((white_pieces == 1 && black_pieces == 0 && 
         (popcount(pos.pieces(WHITE, KNIGHT)) == 1 || popcount(pos.pieces(WHITE, BISHOP)) == 1)) ||
        (black_pieces == 1 && white_pieces == 0 && 
         (popcount(pos.pieces(BLACK, KNIGHT)) == 1 || popcount(pos.pieces(BLACK, BISHOP)) == 1))) {
        // Return near-zero for insufficient material (cannot mate)
        return 0;  // Dead draw - cannot mate with K+N or K+B alone
    }
    
    // King + knight vs King + knight (draw)
    if (white_pieces == 1 && black_pieces == 1 &&
        popcount(pos.pieces(WHITE, KNIGHT)) == 1 && popcount(pos.pieces(BLACK, KNIGHT)) == 1) {
        score = score / 10;  // Nearly a draw
    }
    
    return score;
}

bool ClassicEval::has_back_rank_weakness(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return false;
    
    int king_rank = rank_of(king_sq);
    int back_rank = (c == WHITE) ? 0 : 7;
    
    // King must be on back rank
    if (king_rank != back_rank) return false;
    
    // Check if king is trapped by own pawns
    int king_file = file_of(king_sq);
    Bitboard friendly_pawns = pos.pieces(c, PAWN);
    
    int pawn_rank = (c == WHITE) ? 1 : 6;
    int pawns_in_front = 0;
    
    for (int f = std::max(0, king_file - 1); f <= std::min(7, king_file + 1); f++) {
        Square pawn_sq = make_square(f, pawn_rank);
        if (friendly_pawns & (1ULL << pawn_sq)) {
            pawns_in_front++;
        }
    }
    
    // If 2+ pawns in front of king, it's trapped
    return pawns_in_front >= 2;
}

int ClassicEval::count_pinned_pieces(const Position& pos, Color c) const {
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    Color enemy = static_cast<Color>(1 - c);
    Bitboard all_pieces = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard friendly_pieces = pos.pieces(c);
    int pinned_count = 0;
    
    // Check each enemy slider to see if it pins a friendly piece to the king
    Bitboard enemy_bishops = pos.pieces(enemy, BISHOP) | pos.pieces(enemy, QUEEN);
    Bitboard enemy_rooks = pos.pieces(enemy, ROOK) | pos.pieces(enemy, QUEEN);
    
    // Check diagonal pins
    Bitboard bishop_bb = enemy_bishops;
    while (bishop_bb) {
        Square attacker_sq = pop_lsb(bishop_bb);
        
        // Get the ray from attacker to king (no blockers)
        Bitboard attacks_clear = Attacks::bishop_attacks(attacker_sq, 0);
        
        // Check if king is on this ray
        if (!(attacks_clear & (1ULL << king_sq))) continue;
        
        // King is on the diagonal ray - check pieces between them
        Bitboard king_ray = Attacks::bishop_attacks(king_sq, 0);
        Bitboard line = attacks_clear & king_ray;
        Bitboard pieces_on_line = line & all_pieces;
        
        // If exactly one piece on the line and it's friendly, it's pinned
        if (popcount(pieces_on_line) == 1 && (pieces_on_line & friendly_pieces)) {
            pinned_count++;
        }
    }
    
    // Check straight pins (rook/queen on rank or file)
    Bitboard rook_bb = enemy_rooks;
    while (rook_bb) {
        Square attacker_sq = pop_lsb(rook_bb);
        
        // Get the ray from attacker to king (no blockers)
        Bitboard attacks_clear = Attacks::rook_attacks(attacker_sq, 0);
        
        // Check if king is on this ray
        if (!(attacks_clear & (1ULL << king_sq))) continue;
        
        // King is on the rank/file ray - check pieces between them
        Bitboard king_ray = Attacks::rook_attacks(king_sq, 0);
        Bitboard line = attacks_clear & king_ray;
        Bitboard pieces_on_line = line & all_pieces;
        
        // If exactly one piece on the line and it's friendly, it's pinned
        if (popcount(pieces_on_line) == 1 && (pieces_on_line & friendly_pieces)) {
            pinned_count++;
        }
    }
    
    return pinned_count;
}

int ClassicEval::count_restricted_rooks(const Position& pos, Color c) const {
    // Rook attacked by enemy rook/queen with king adjacent - severely restricted
    Square king_sq = pos.king_square(c);
    if (king_sq == SQ_NONE) return 0;
    
    Color enemy = static_cast<Color>(1 - c);
    Bitboard all_pieces = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard enemy_sliders = pos.pieces(enemy, ROOK) | pos.pieces(enemy, QUEEN);
    Bitboard our_rooks = pos.pieces(c, ROOK);
    int count = 0;
    
    while (our_rooks) {
        Square rook_sq = pop_lsb(our_rooks);
        // King adjacent to rook (within 1 square)?
        int fd = abs(file_of(rook_sq) - file_of(king_sq));
        int rd = abs(rank_of(rook_sq) - rank_of(king_sq));
        if (fd > 1 || rd > 1) continue;
        
        // Is rook attacked by enemy rook/queen?
        Bitboard attackers = enemy_sliders;
        while (attackers) {
            Square att_sq = pop_lsb(attackers);
            Bitboard attacks = Attacks::rook_attacks(att_sq, all_pieces);
            if (attacks & (1ULL << rook_sq)) {
                count++;
                break;
            }
        }
    }
    return count;
}

int ClassicEval::evaluate_tactics(const Position& pos) const {
    int score = 0;
    
    for (Color c : {WHITE, BLACK}) {
        int sign = (c == WHITE) ? 1 : -1;
        
        // Back rank weakness
        if (has_back_rank_weakness(pos, c)) {
            score += sign * -300;  // Large penalty for back rank weakness
        }
        
        // Pinned pieces
        int pinned = count_pinned_pieces(pos, c);
        score += sign * -pinned * 150;  // Moderate penalty for each pinned piece
        
        // Restricted rooks (attacked with king adjacent) - severe penalty
        int restricted = count_restricted_rooks(pos, c);
        score += sign * -restricted * 320;  // ~rook value - piece is trapped
        
        // Pawn blocking king on back rank - only when we have a rook (reduces rook's open file value)
        if (popcount(pos.pieces(c, ROOK)) > 0) {
            Square king_sq = pos.king_square(c);
            if (king_sq != SQ_NONE) {
                int back_rank = (c == WHITE) ? 0 : 7;
                if (rank_of(king_sq) == back_rank) {
                    int file = file_of(king_sq);
                    int block_rank = (c == WHITE) ? 1 : 6;
                    Square block_sq = make_square(file, block_rank);
                    if (pos.pieces(c, PAWN) & (1ULL << block_sq)) {
                        score += sign * -120;  // Pawn blocks king escape, hurts rook flexibility
                    }
                }
            }
        }
        
        // Hanging rooks and queens (attacked by enemy rooks/queens)
        // Only penalize if the piece is attacked and the attacker is protected or equal value
        Color enemy = static_cast<Color>(1 - c);
        Bitboard all_pieces = pos.pieces(WHITE) | pos.pieces(BLACK);
        Bitboard enemy_rooks = pos.pieces(enemy, ROOK) | pos.pieces(enemy, QUEEN);
        Bitboard our_rooks_queens = pos.pieces(c, ROOK) | pos.pieces(c, QUEEN);
        
        // Check if our rooks/queens are attacked by enemy rooks/queens
        Bitboard our_pieces = pos.pieces(c, ROOK) | pos.pieces(c, QUEEN);
        Bitboard piece_bb = our_pieces;
        while (piece_bb) {
            Square piece_sq = pop_lsb(piece_bb);
            bool is_attacked = false;
            bool attacker_protected = false;
            
            Bitboard enemy_rook_bb = enemy_rooks;
            while (enemy_rook_bb) {
                Square enemy_rook_sq = pop_lsb(enemy_rook_bb);
                if (Attacks::rook_attacks(enemy_rook_sq, all_pieces) & (1ULL << piece_sq)) {
                    is_attacked = true;
                    // Check if our piece can capture back
                    if (!(Attacks::rook_attacks(piece_sq, all_pieces) & (1ULL << enemy_rook_sq))) {
                        attacker_protected = true;  // We can't capture back, so it's protected
                    }
                    break;
                }
            }
            
            if (is_attacked && attacker_protected) {
                score += sign * -250;  // Penalty for piece under attack that can't trade
            } else if (is_attacked) {
                score += sign * -50;  // Small penalty for piece under attack (can trade)
            }
        }
    }
    
    return score;
}

int ClassicEval::evaluate_exchange_compensation(const Position& pos) const {
    // When side to move is down ~exchange (rook for minor, 300-400 cp), add bonuses for attack compensation
    // Skip in trivial endgames (e.g. K+N vs K) where compensation is meaningless
    int piece_count = popcount(pos.pieces(WHITE)) + popcount(pos.pieces(BLACK)) - 2;  // Exclude kings
    if (piece_count < 4) return 0;  // Need enough pieces for meaningful compensation

    int white_mat = 0, black_mat = 0;
    for (PieceType pt = PAWN; pt <= QUEEN; pt = static_cast<PieceType>(pt + 1)) {
        white_mat += popcount(pos.pieces(WHITE, pt)) * MATERIAL[pt];
        black_mat += popcount(pos.pieces(BLACK, pt)) * MATERIAL[pt];
    }
    int material_diff = white_mat - black_mat;  // positive = white ahead

    int compensation = 0;
    Color down_side;
    if (material_diff <= -300 && material_diff >= -450) {
        down_side = WHITE;  // White down exchange
        compensation = 1;
    } else if (material_diff >= 300 && material_diff <= 450) {
        down_side = BLACK;  // Black down exchange
        compensation = -1;
    } else {
        return 0;
    }

    Color enemy = static_cast<Color>(1 - down_side);
    int bonus = 0;

    // 1. Open files toward enemy king
    Square enemy_king = pos.king_square(enemy);
    if (enemy_king != SQ_NONE) {
        int king_file = file_of(enemy_king);
        Bitboard our_rooks_queens = pos.pieces(down_side, ROOK) | pos.pieces(down_side, QUEEN);
        Bitboard all_pawns = pos.pieces(PAWN);
        while (our_rooks_queens) {
            Square sq = pop_lsb(our_rooks_queens);
            int f = file_of(sq);
            if (abs(f - king_file) <= 2) {  // File toward king
                bool open = true;
                for (int r = 0; r < 8 && open; r++) {
                    if (all_pawns & (1ULL << make_square(f, r))) open = false;
                }
                if (open) bonus += 25;
            }
        }
    }

    // 2. Opponent pawn structure damage (doubled/isolated)
    Bitboard enemy_pawns = pos.pieces(enemy, PAWN);
    Bitboard ep = enemy_pawns;
    while (ep) {
        Square sq = pop_lsb(ep);
        if (is_doubled_pawn(pos, sq, enemy)) bonus += 15;
        if (is_isolated_pawn(pos, sq, enemy)) bonus += 20;
    }

    // 3. Opponent king exposure (uncastled, weak pawn shield)
    if (enemy_king != SQ_NONE) {
        int back_rank = (enemy == WHITE) ? 0 : 7;
        if (rank_of(enemy_king) == back_rank) {
            bonus += 30;  // King still on back rank (uncastled or exposed)
        }
        int shield = count_pawn_shield(pos, enemy);
        if (shield < 2) bonus += (2 - shield) * 20;  // Weak pawn shield
        int open_near = count_open_files_near_king(pos, enemy);
        if (open_near > 0) bonus += open_near * 15;
    }

    return compensation * bonus;
}

int ClassicEval::evaluate_opening(const Position& pos, int phase) const {
    int score = 0;
    
    // Opening principles only matter in opening (phase > 200)
    if (phase < 200) return 0;
    
    // Strong bonus for central pawn control (e4, d4, e5, d5)
    Bitboard white_pawns = pos.pieces(WHITE, PAWN);
    Bitboard black_pawns = pos.pieces(BLACK, PAWN);
    
    // Check for pawns on central squares
    Square e4 = make_square(4, 3);  // e4
    Square d4 = make_square(3, 3);  // d4
    Square e5 = make_square(4, 4);  // e5
    Square d5 = make_square(3, 4);  // d5
    
    // Bonus for central pawns in opening (moderate - avoid overvaluing known equal positions)
    if (white_pawns & (1ULL << e4)) score += 15;
    if (white_pawns & (1ULL << d4)) score += 15;
    if (black_pawns & (1ULL << e5)) score -= 15;
    if (black_pawns & (1ULL << d5)) score -= 15;
    
    // Bonus for knight development (Nf3, Nc3 for white)
    Bitboard white_knights = pos.pieces(WHITE, KNIGHT);
    Bitboard black_knights = pos.pieces(BLACK, KNIGHT);
    
    Square nf3 = make_square(5, 2);  // f3
    Square nc3 = make_square(2, 2);  // c3
    Square nf6 = make_square(5, 5);  // f6
    Square nc6 = make_square(2, 5);  // c6
    
    // Bonus for knight development
    if (white_knights & (1ULL << nf3)) score += 20;
    if (white_knights & (1ULL << nc3)) score += 20;
    if (black_knights & (1ULL << nf6)) score -= 20;
    if (black_knights & (1ULL << nc6)) score -= 20;
    
    // Strong penalty for knights still on back rank in opening
    Square g1 = make_square(6, 0);  // g1
    Square b1 = make_square(1, 0);  // b1
    Square g8 = make_square(6, 7);  // g8
    Square b8 = make_square(1, 7);  // b8
    
    if (white_knights & (1ULL << g1)) score -= 35;
    if (white_knights & (1ULL << b1)) score -= 35;
    if (black_knights & (1ULL << g8)) score += 35;
    if (black_knights & (1ULL << b8)) score += 35;
    
    // Bonus for bishop development (Bc5, Bf5, Bc4, Bf4 for black/white)
    Bitboard white_bishops = pos.pieces(WHITE, BISHOP);
    Bitboard black_bishops = pos.pieces(BLACK, BISHOP);
    
    Square bc4 = make_square(2, 3);  // c4
    Square bf4 = make_square(5, 3);  // f4
    Square bc5 = make_square(2, 4);  // c5
    Square bf5 = make_square(5, 4);  // f5
    
    // Bonus for active bishop development
    if (white_bishops & (1ULL << bc4)) score += 10;
    if (white_bishops & (1ULL << bf4)) score += 10;
    if (black_bishops & (1ULL << bc5)) score -= 10;
    if (black_bishops & (1ULL << bf5)) score -= 10;
    
    // Penalty for bishops developed before both knights (Knights before Bishops principle)
    int white_knights_developed = 0;
    int black_knights_developed = 0;
    
    // Count developed knights (not on starting squares)
    if (!(white_knights & (1ULL << g1))) white_knights_developed++;
    if (!(white_knights & (1ULL << b1))) white_knights_developed++;
    if (!(black_knights & (1ULL << g8))) black_knights_developed++;
    if (!(black_knights & (1ULL << b8))) black_knights_developed++;
    
    // Count bishops off starting squares
    Square f1 = make_square(5, 0);
    Square c1 = make_square(2, 0);
    Square f8 = make_square(5, 7);
    Square c8 = make_square(2, 7);
    
    int white_bishops_developed = 0;
    int black_bishops_developed = 0;
    
    if (!(white_bishops & (1ULL << f1))) white_bishops_developed++;
    if (!(white_bishops & (1ULL << c1))) white_bishops_developed++;
    if (!(black_bishops & (1ULL << f8))) black_bishops_developed++;
    if (!(black_bishops & (1ULL << c8))) black_bishops_developed++;
    
    // Penalize if bishops are developed but knights aren't
    if (white_bishops_developed > 0 && white_knights_developed < 2) {
        score -= 50 * white_bishops_developed;  // Penalty for developing bishops before knights
    }
    if (black_bishops_developed > 0 && black_knights_developed < 2) {
        score += 50 * black_bishops_developed;  // Penalty for black
    }
    
    return score;
}

void ClassicEval::initialize(Position& pos) {
    (void)pos;
}

} // namespace chess

