#include "nnue_eval.h"
#include "nnue_loader.h"
#include "../board/position.h"
#include "../board/bitboard.h"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace chess {

NnueEval::NnueEval() {
    // Initialize network structure
    network.input_size = FEATURES_PER_KING;
    network.hidden_size = 256;  // Typical NNUE hidden layer size
    network.output_size = 1;
    network.loaded = false;
    
    // Create default network for now (can be replaced by loading from file)
    create_default_network();
}

NnueEval::~NnueEval() {
    network.cleanup();
}

int NnueEval::feature_index(Square king_sq, Square piece_sq, PieceType pt, Color piece_color) {
    if (pt == KING || pt == NO_PIECE) return -1;
    
    int pt_idx = pt - 1; // PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4
    int color_idx = piece_color;
    int piece_offset = (piece_sq * PIECE_TYPES + pt_idx) * 2 + color_idx;
    
    // Stockfish HalfKP uses 641 features per king (41024 total); our format uses 640 (40960)
    if (use_stockfish_features) {
        return king_sq * 641 + piece_offset;
    }
    return king_sq * FEATURES_PER_KING + piece_offset;
}

void NnueEval::extract_features(const Position& pos, Color perspective, std::vector<int>& features) {
    features.clear();
    
    Square king_sq = pos.king_square(perspective);
    if (king_sq == SQ_NONE) return;
    
    // Extract features for all pieces relative to the king
    for (Color c : {WHITE, BLACK}) {
        for (PieceType pt = PAWN; pt <= QUEEN; pt = static_cast<PieceType>(pt + 1)) {
            Bitboard bb = pos.pieces(c, pt);
            while (bb) {
                Square sq = pop_lsb(bb);
                int idx = feature_index(king_sq, sq, pt, c);
                if (idx >= 0) {
                    features.push_back(idx);
                }
            }
        }
    }
}

void NnueEval::refresh_accumulator(Position& pos, Color side) {
    // Recompute accumulator from scratch
    Accumulator* acc = static_cast<Accumulator*>(pos.nnue.acc[side]);
    if (!acc) {
        acc = new Accumulator();
        acc->values = new int16_t[network.hidden_size];  // Accumulator is hidden layer size
        pos.nnue.acc[side] = acc;
    }
    
    if (!network.loaded) {
        acc->dirty = false;
        return;
    }
    
    // Zero accumulator (hidden layer)
    std::memset(acc->values, 0, network.hidden_size * sizeof(int16_t));
    
    // Extract features and accumulate weights
    std::vector<int> features;
    extract_features(pos, side, features);
    
    // Accumulate: for each active feature, add its weights to accumulator
    for (int feat : features) {
        if (feat >= 0 && feat < network.input_size) {
            // Add input weights for this feature to accumulator
            int offset = feat * network.hidden_size;
            for (int i = 0; i < network.hidden_size; i++) {
                acc->values[i] += network.input_weights[offset + i];
            }
        }
    }
    
    // Add hidden bias
    for (int i = 0; i < network.hidden_size; i++) {
        acc->values[i] += network.hidden_bias[i];
    }
    
    acc->dirty = false;
}

int16_t* NnueEval::get_accumulator(Position& pos, Color side) {
    Accumulator* acc = static_cast<Accumulator*>(pos.nnue.acc[side]);
    if (!acc || acc->dirty) {
        refresh_accumulator(pos, side);
        acc = static_cast<Accumulator*>(pos.nnue.acc[side]);
    }
    return acc->values;
}

void NnueEval::update_accumulator_incremental(Position& pos, Color side, const UndoInfo::NnueDelta& delta) {
    Accumulator* acc = static_cast<Accumulator*>(pos.nnue.acc[side]);
    if (!acc || !network.loaded) {
        refresh_accumulator(pos, side);
        return;
    }
    
    Square king_sq = pos.king_square(side);
    if (king_sq == SQ_NONE) {
        refresh_accumulator(pos, side);
        return;
    }
    
    // Helper to subtract feature weights
    auto remove_feature = [&](int feat_idx) {
        if (feat_idx >= 0 && feat_idx < network.input_size) {
            int offset = feat_idx * network.hidden_size;
            for (int i = 0; i < network.hidden_size; i++) {
                acc->values[i] -= network.input_weights[offset + i];
            }
        }
    };
    
    // Helper to add feature weights
    auto add_feature = [&](int feat_idx) {
        if (feat_idx >= 0 && feat_idx < network.input_size) {
            int offset = feat_idx * network.hidden_size;
            for (int i = 0; i < network.hidden_size; i++) {
                acc->values[i] += network.input_weights[offset + i];
            }
        }
    };
    
    // Remove old feature
    if (delta.moved_piece != NO_PIECE_PIECE) {
        PieceType pt = type_of(delta.moved_piece);
        Color pc = color_of(delta.moved_piece);
        int idx = feature_index(king_sq, delta.moved_from, pt, pc);
        remove_feature(idx);
    }
    
    // Add new feature
    if (delta.moved_piece != NO_PIECE_PIECE) {
        PieceType pt = type_of(delta.moved_piece);
        Color pc = color_of(delta.moved_piece);
        int idx = feature_index(king_sq, delta.moved_to, pt, pc);
        add_feature(idx);
    }
    
    // Handle captured piece
    if (delta.captured_piece != NO_PIECE_PIECE) {
        PieceType pt = type_of(delta.captured_piece);
        Color pc = color_of(delta.captured_piece);
        int idx = feature_index(king_sq, delta.captured_square, pt, pc);
        remove_feature(idx);
    }
    
    // Handle promotion (remove pawn, add promoted piece)
    if (delta.promo_piece != NO_PIECE_PIECE) {
        // Remove pawn at promo square
        int pawn_idx = feature_index(king_sq, delta.promo_square, PAWN, side);
        remove_feature(pawn_idx);
        // Add promoted piece
        PieceType pt = type_of(delta.promo_piece);
        Color pc = color_of(delta.promo_piece);
        int promo_idx = feature_index(king_sq, delta.promo_square, pt, pc);
        add_feature(promo_idx);
    }
    
    // Handle castling (rook move)
    if (delta.is_castling) {
        int idx_from = feature_index(king_sq, delta.rook_from, ROOK, side);
        int idx_to = feature_index(king_sq, delta.rook_to, ROOK, side);
        remove_feature(idx_from);
        add_feature(idx_to);
    }
    
    // Handle en passant
    if (delta.is_en_passant) {
        PieceType pt = PAWN;
        Color pc = static_cast<Color>(1 - side);
        int idx = feature_index(king_sq, delta.ep_captured_square, pt, pc);
        remove_feature(idx);
    }
}

int NnueEval::forward_pass(const int16_t* accumulator, Color perspective) {
    if (!network.loaded) {
        return 0;
    }
    
    // Clipped ReLU activation on hidden layer
    int32_t hidden[256];  // Use int32_t for intermediate calculations
    for (int i = 0; i < network.hidden_size; i++) {
        int32_t val = accumulator[i];
        // Clipped ReLU: clamp between 0 and 127
        hidden[i] = (val < 0) ? 0 : ((val > 127) ? 127 : val);
    }
    
    // Output layer: dot product with output weights + bias
    int32_t output = network.output_bias[0];
    for (int i = 0; i < network.hidden_size; i++) {
        output += hidden[i] * network.output_weights[i];
    }
    
    // Scale down (weights are typically in fixed point)
    int score = output / 64;  // Adjust scale factor based on weight quantization
    
    // Return score from perspective
    return (perspective == WHITE) ? score : -score;
}

int NnueEval::evaluate(const Position& pos) {
    Position& non_const_pos = const_cast<Position&>(pos);
    if (!non_const_pos.nnue.initialized) {
        initialize(non_const_pos);
    }
    
    Color stm = pos.side_to_move();
    int16_t* acc = get_accumulator(non_const_pos, stm);
    
    // Forward pass through network
    int score = forward_pass(acc, stm);
    
    // Return score from side to move's perspective
    return score;
}

void NnueEval::on_make_move(Position& pos, Move move, const UndoInfo& undo) {
    // Note: side_to_move has already been flipped in make_move
    // So pos.side_to_move() is now the OPPONENT of who just moved
    
    // Update accumulator for the side that just moved (now opponent's turn)
    Color moved_side = static_cast<Color>(1 - pos.side_to_move());
    update_accumulator_incremental(pos, moved_side, undo.nnue_delta);
    
    // Update accumulator for the current side (pieces changed due to capture/move)
    update_accumulator_incremental(pos, pos.side_to_move(), undo.nnue_delta);
}

void NnueEval::on_unmake_move(Position& pos, Move move, const UndoInfo& undo) {
    // Reverse the incremental update
    // For simplicity, mark accumulators as dirty and refresh on next evaluation
    // In optimized version, would reverse the delta operations
    
    Accumulator* acc_w = static_cast<Accumulator*>(pos.nnue.acc[WHITE]);
    Accumulator* acc_b = static_cast<Accumulator*>(pos.nnue.acc[BLACK]);
    
    if (acc_w) acc_w->dirty = true;
    if (acc_b) acc_b->dirty = true;
}

void NnueEval::initialize(Position& pos) {
    // Initialize accumulators for both sides
    refresh_accumulator(pos, WHITE);
    refresh_accumulator(pos, BLACK);
    pos.nnue.initialized = true;
}

bool NnueEval::load_network(const std::string& filename) {
    // Clean up old network
    network.cleanup();
    use_stockfish_features = false;
    
    bool success = NnueLoader::load_from_file(
        filename,
        network.input_weights,
        network.hidden_bias,
        network.output_weights,
        network.output_bias,
        network.input_size,
        network.hidden_size,
        network.output_size
    );
    
    if (success) {
        network.loaded = true;
        use_stockfish_features = (network.input_size == 41024);  // Stockfish HalfKP
        std::cout << "Loaded NNUE network: " << filename << std::endl;
        std::cout << "  Input size: " << network.input_size << std::endl;
        std::cout << "  Hidden size: " << network.hidden_size << std::endl;
        std::cout << "  Output size: " << network.output_size << std::endl;
    } else {
        network.loaded = false;
        std::cerr << "Failed to load NNUE network, using default" << std::endl;
        create_default_network();
    }
    
    return success;
}

void NnueEval::create_default_network() {
    // Clean up old network
    network.cleanup();
    
    NnueLoader::create_default_network(
        network.input_weights,
        network.hidden_bias,
        network.output_weights,
        network.output_bias,
        network.input_size,
        network.hidden_size,
        network.output_size
    );
    
    network.loaded = true;
    std::cout << "Created default NNUE network (random weights)" << std::endl;
}

} // namespace chess

