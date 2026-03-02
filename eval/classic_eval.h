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
    
    // Export evaluation parameters for tuning
    struct EvalParams {
        int material_pawn;
        int material_knight;
        int material_bishop;
        int material_rook;
        int material_queen;
        
        int doubled_pawn_penalty;
        int isolated_pawn_penalty;
        int backward_pawn_penalty;
        int passed_pawn_bonus[8];
        int pawn_chain_bonus;
        
        int rook_open_file_bonus;
        int rook_semi_open_file_bonus;
        int rook_blocked_penalty;
        int bishop_pair_bonus;
        int knight_outpost_bonus;
        int bad_bishop_penalty;
        int passive_bishop_penalty;
        
        int pawn_shield_bonus;
        int open_file_near_king_penalty;
        int king_tropism_weight;
        int castling_bonus;
        int king_center_penalty;
        int king_tropism_uncastled_multiplier;  // 100 = 1.0x, 150 = 1.5x when king in center
        
        int mobility_weight[6];
        
        int king_activity_bonus;
        int opposition_bonus;
    };
    
    const EvalParams& get_params() const { return params; }
    void set_params(const EvalParams& p) { params = p; }

private:
    // Piece-square tables
    static const int PST_PAWN[64];
    static const int PST_KNIGHT[64];
    static const int PST_BISHOP[64];
    static const int PST_ROOK[64];
    static const int PST_QUEEN[64];
    static const int PST_QUEEN_OPENING[64];
    static const int PST_KING[64];
    static const int PST_KING_END[64];
    
    // Material values
    static const int MATERIAL[7];
    
    // Evaluation parameters (tunable)
    EvalParams params;
    
    // Game phase detection
    int calculate_game_phase(const Position& pos) const;
    int tapered_eval(int mg_score, int eg_score, int phase) const;
    
    // Main evaluation components
    int pst_value(PieceType pt, Square sq, Color c, int phase) const;
    int evaluate_pieces(const Position& pos, int phase) const;
    int evaluate_pawns(const Position& pos) const;
    int evaluate_king_safety(const Position& pos, int phase) const;
    int evaluate_mobility(const Position& pos) const;
    int evaluate_piece_specific(const Position& pos) const;
    int evaluate_endgame(const Position& pos, int phase) const;
    int evaluate_tactics(const Position& pos) const;
    int evaluate_exchange_compensation(const Position& pos) const;
    int evaluate_opening(const Position& pos, int phase) const;
    
    // Pawn structure helpers
    bool is_doubled_pawn(const Position& pos, Square sq, Color c) const;
    bool is_isolated_pawn(const Position& pos, Square sq, Color c) const;
    bool is_passed_pawn(const Position& pos, Square sq, Color c) const;
    bool is_backward_pawn(const Position& pos, Square sq, Color c) const;
    bool is_pawn_chain(const Position& pos, Square sq, Color c) const;
    
    // King safety helpers
    int count_pawn_shield(const Position& pos, Color c) const;
    int count_open_files_near_king(const Position& pos, Color c) const;
    int calculate_king_tropism(const Position& pos, Color c) const;
    
    // Piece-specific helpers
    bool is_rook_on_open_file(const Position& pos, Square sq) const;
    bool is_rook_on_semi_open_file(const Position& pos, Square sq, Color c) const;
    bool is_rook_blocked_by_pawn(const Position& pos, Square sq, Color c) const;
    bool has_bishop_pair(const Position& pos, Color c) const;
    bool is_knight_outpost(const Position& pos, Square sq, Color c) const;
    bool is_bad_bishop(const Position& pos, Square sq, Color c) const;
    
    // Endgame helpers
    int king_activity_score(const Position& pos, Color c) const;
    bool has_opposition(const Position& pos) const;
    
    // Tactical helpers
    bool has_back_rank_weakness(const Position& pos, Color c) const;
    int count_pinned_pieces(const Position& pos, Color c) const;
    int count_restricted_rooks(const Position& pos, Color c) const;
    
    // Bitboard helpers
    Bitboard get_passed_pawn_mask(Square sq, Color c) const;
    Bitboard get_isolated_pawn_mask(Square sq) const;
    Bitboard get_backward_pawn_mask(Square sq, Color c) const;
};

} // namespace chess

