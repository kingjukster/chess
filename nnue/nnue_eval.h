#pragma once

#include "../eval/evaluator.h"
#include "../board/types.h"
#include <string>

namespace chess {

// NNUE evaluator using HalfKP feature set
// This is the main evaluation backend
class NnueEval : public Evaluator {
public:
    NnueEval();
    ~NnueEval() override;
    
    int evaluate(const Position& pos) override;
    void on_make_move(Position& pos, Move move, const UndoInfo& undo) override;
    void on_unmake_move(Position& pos, Move move, const UndoInfo& undo) override;
    void initialize(Position& pos) override;
    
    // Load network weights from file
    bool load_network(const std::string& filename);
    
    // Create default network (for testing)
    void create_default_network();

private:
    // Feature extraction (HalfKP style)
    // Features: king_square * 64 + piece_square + piece_type_offset
    static constexpr int KING_BUCKETS = 64;
    static constexpr int PIECE_SQUARES = 64;
    static constexpr int PIECE_TYPES = 5; // PAWN, KNIGHT, BISHOP, ROOK, QUEEN (no KING)
    static constexpr int FEATURES_PER_KING = PIECE_SQUARES * PIECE_TYPES * 2; // 2 colors
    static constexpr int TOTAL_FEATURES = KING_BUCKETS * FEATURES_PER_KING;
    
    // Accumulator structure (per side)
    struct Accumulator {
        int16_t* values;  // Accumulator values
        bool dirty;       // Needs recomputation
        
        Accumulator() : values(nullptr), dirty(true) {}
    };
    
    // Network structure
    struct Network {
        int16_t* input_weights;   // Input layer weights [input_size * hidden_size]
        int16_t* hidden_bias;      // Hidden layer bias [hidden_size]
        int16_t* output_weights;  // Output layer weights [hidden_size * output_size]
        int16_t* output_bias;     // Output layer bias [output_size]
        int input_size;
        int hidden_size;
        int output_size;
        bool loaded;
        
        Network() : input_weights(nullptr), hidden_bias(nullptr), 
                   output_weights(nullptr), output_bias(nullptr),
                   input_size(0), hidden_size(0), output_size(0), loaded(false) {}
        
        void cleanup() {
            delete[] input_weights;
            delete[] hidden_bias;
            delete[] output_weights;
            delete[] output_bias;
            input_weights = hidden_bias = output_weights = output_bias = nullptr;
        }
    };
    
    Network network;
    
    // Stockfish format uses 641 features per king (vs our 640); set when loading .nnue
    bool use_stockfish_features = false;
    
    // Feature extraction
    void extract_features(const Position& pos, Color perspective, std::vector<int>& features);
    void update_accumulator_incremental(Position& pos, Color side, const UndoInfo::NnueDelta& delta);
    void refresh_accumulator(Position& pos, Color side);
    int16_t* get_accumulator(Position& pos, Color side);
    
    // Network inference
    int forward_pass(const int16_t* accumulator, Color perspective);
    
    // Helper functions
    int feature_index(Square king_sq, Square piece_sq, PieceType pt, Color piece_color);
};

} // namespace chess

