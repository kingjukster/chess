#include "tuner.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace chess {

TexelTuner::TexelTuner() : evaluator(nullptr), current_error(0.0) {
}

bool TexelTuner::load_training_data(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open training data: " << filename << std::endl;
        return false;
    }
    
    positions.clear();
    std::string line;
    int loaded = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string fen_part;
        std::vector<std::string> fen_parts;
        
        // Read FEN (6 parts)
        for (int i = 0; i < 6; i++) {
            std::string part;
            if (!(iss >> part)) break;
            fen_parts.push_back(part);
        }
        
        if (fen_parts.size() < 6) continue;
        
        std::string fen = fen_parts[0] + " " + fen_parts[1] + " " + fen_parts[2] + " " +
                         fen_parts[3] + " " + fen_parts[4] + " " + fen_parts[5];
        
        // Read result
        double result;
        if (!(iss >> result)) continue;
        
        TrainingPosition tp;
        tp.pos.from_fen(fen);
        tp.result = result;
        positions.push_back(tp);
        loaded++;
    }
    
    file.close();
    std::cout << "Loaded " << loaded << " training positions" << std::endl;
    return loaded > 0;
}

double TexelTuner::sigmoid(double score, double k) const {
    // Convert centipawn score to win probability
    // k parameter controls the scaling (typically around 1.0-2.0)
    return 1.0 / (1.0 + std::exp(-k * score / 400.0));
}

double TexelTuner::calculate_error() {
    if (!evaluator || positions.empty()) return 0.0;
    
    double error = 0.0;
    
    for (const auto& tp : positions) {
        evaluator->initialize(const_cast<Position&>(tp.pos));
        int eval = evaluator->evaluate(tp.pos);
        double predicted = sigmoid(eval);
        double diff = predicted - tp.result;
        error += diff * diff;
    }
    
    // Mean squared error
    return error / positions.size();
}

std::vector<int*> TexelTuner::get_tunable_parameters() {
    if (!evaluator) return {};
    
    ClassicEval::EvalParams& params = const_cast<ClassicEval::EvalParams&>(evaluator->get_params());
    std::vector<int*> tunable;
    
    // Material values
    tunable.push_back(&params.material_pawn);
    tunable.push_back(&params.material_knight);
    tunable.push_back(&params.material_bishop);
    tunable.push_back(&params.material_rook);
    tunable.push_back(&params.material_queen);
    
    // Pawn structure
    tunable.push_back(&params.doubled_pawn_penalty);
    tunable.push_back(&params.isolated_pawn_penalty);
    tunable.push_back(&params.backward_pawn_penalty);
    tunable.push_back(&params.pawn_chain_bonus);
    
    // Passed pawn bonuses (ranks 2-7)
    for (int i = 2; i < 8; i++) {
        tunable.push_back(&params.passed_pawn_bonus[i]);
    }
    
    // Piece-specific
    tunable.push_back(&params.rook_open_file_bonus);
    tunable.push_back(&params.rook_semi_open_file_bonus);
    tunable.push_back(&params.bishop_pair_bonus);
    tunable.push_back(&params.knight_outpost_bonus);
    tunable.push_back(&params.bad_bishop_penalty);
    
    // King safety
    tunable.push_back(&params.pawn_shield_bonus);
    tunable.push_back(&params.open_file_near_king_penalty);
    tunable.push_back(&params.king_tropism_weight);
    tunable.push_back(&params.castling_bonus);
    tunable.push_back(&params.king_center_penalty);
    tunable.push_back(&params.king_tropism_uncastled_multiplier);
    
    // Mobility
    tunable.push_back(&params.mobility_weight[KNIGHT]);
    tunable.push_back(&params.mobility_weight[BISHOP]);
    tunable.push_back(&params.mobility_weight[ROOK]);
    tunable.push_back(&params.mobility_weight[QUEEN]);
    
    // Endgame
    tunable.push_back(&params.king_activity_bonus);
    tunable.push_back(&params.opposition_bonus);
    
    return tunable;
}

std::vector<std::string> TexelTuner::get_parameter_names() {
    std::vector<std::string> names;
    
    names.push_back("material_pawn");
    names.push_back("material_knight");
    names.push_back("material_bishop");
    names.push_back("material_rook");
    names.push_back("material_queen");
    
    names.push_back("doubled_pawn_penalty");
    names.push_back("isolated_pawn_penalty");
    names.push_back("backward_pawn_penalty");
    names.push_back("pawn_chain_bonus");
    
    for (int i = 2; i < 8; i++) {
        names.push_back("passed_pawn_bonus_rank" + std::to_string(i));
    }
    
    names.push_back("rook_open_file_bonus");
    names.push_back("rook_semi_open_file_bonus");
    names.push_back("bishop_pair_bonus");
    names.push_back("knight_outpost_bonus");
    names.push_back("bad_bishop_penalty");
    
    names.push_back("pawn_shield_bonus");
    names.push_back("open_file_near_king_penalty");
    names.push_back("king_tropism_weight");
    names.push_back("castling_bonus");
    names.push_back("king_center_penalty");
    names.push_back("king_tropism_uncastled_multiplier");
    
    names.push_back("mobility_knight");
    names.push_back("mobility_bishop");
    names.push_back("mobility_rook");
    names.push_back("mobility_queen");
    
    names.push_back("king_activity_bonus");
    names.push_back("opposition_bonus");
    
    return names;
}

void TexelTuner::calculate_gradients(std::vector<double>& gradients) {
    if (!evaluator || positions.empty()) return;
    
    auto params = get_tunable_parameters();
    gradients.resize(params.size(), 0.0);
    
    const double epsilon = 1.0;
    
    for (size_t i = 0; i < params.size(); i++) {
        int original_value = *params[i];
        
        // Evaluate with parameter + epsilon
        *params[i] = original_value + static_cast<int>(epsilon);
        double error_plus = calculate_error();
        
        // Evaluate with parameter - epsilon
        *params[i] = original_value - static_cast<int>(epsilon);
        double error_minus = calculate_error();
        
        // Restore original value
        *params[i] = original_value;
        
        // Calculate gradient (finite difference)
        gradients[i] = (error_plus - error_minus) / (2.0 * epsilon);
    }
}

void TexelTuner::tune(int iterations, double learning_rate) {
    if (!evaluator || positions.empty()) {
        std::cerr << "Tuner not properly initialized" << std::endl;
        return;
    }
    
    std::cout << "Starting Texel tuning with " << positions.size() << " positions" << std::endl;
    std::cout << "Iterations: " << iterations << ", Learning rate: " << learning_rate << std::endl;
    
    auto params = get_tunable_parameters();
    auto param_names = get_parameter_names();
    
    current_error = calculate_error();
    std::cout << "Initial error: " << current_error << std::endl;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Calculate gradients
        std::vector<double> gradients;
        calculate_gradients(gradients);
        
        // Update parameters using gradient descent
        for (size_t i = 0; i < params.size(); i++) {
            double update = -learning_rate * gradients[i];
            *params[i] += static_cast<int>(std::round(update));
            
            // Ensure parameters stay reasonable
            if (*params[i] < 0) *params[i] = 0;
            if (*params[i] > 10000) *params[i] = 10000;
        }
        
        // Calculate new error
        double new_error = calculate_error();
        
        // Print progress every 10 iterations
        if (iter % 10 == 0 || iter == iterations - 1) {
            std::cout << "Iteration " << iter << ": error = " << new_error;
            if (new_error < current_error) {
                std::cout << " (improved by " << (current_error - new_error) << ")";
            }
            std::cout << std::endl;
        }
        
        // Adaptive learning rate
        if (new_error > current_error) {
            learning_rate *= 0.95;
        } else if (new_error < current_error * 0.99) {
            learning_rate *= 1.05;
        }
        
        current_error = new_error;
        
        // Early stopping if error is very small
        if (current_error < 0.001) {
            std::cout << "Converged at iteration " << iter << std::endl;
            break;
        }
    }
    
    std::cout << "Tuning complete. Final error: " << current_error << std::endl;
    
    // Print final parameters
    std::cout << "\nTuned parameters:" << std::endl;
    for (size_t i = 0; i < params.size(); i++) {
        std::cout << "  " << param_names[i] << " = " << *params[i] << std::endl;
    }
}

void TexelTuner::local_search(int iterations) {
    if (!evaluator || positions.empty()) return;
    
    auto params = get_tunable_parameters();
    current_error = calculate_error();
    
    std::cout << "Starting local search optimization" << std::endl;
    std::cout << "Initial error: " << current_error << std::endl;
    
    for (int iter = 0; iter < iterations; iter++) {
        bool improved = false;
        
        // Try adjusting each parameter by +/- 1
        for (size_t i = 0; i < params.size(); i++) {
            int original_value = *params[i];
            
            // Try +1
            *params[i] = original_value + 1;
            double error_plus = calculate_error();
            
            if (error_plus < current_error) {
                current_error = error_plus;
                improved = true;
                continue;
            }
            
            // Try -1
            *params[i] = original_value - 1;
            if (*params[i] >= 0) {
                double error_minus = calculate_error();
                
                if (error_minus < current_error) {
                    current_error = error_minus;
                    improved = true;
                    continue;
                }
            }
            
            // Restore original value
            *params[i] = original_value;
        }
        
        if (iter % 10 == 0) {
            std::cout << "Iteration " << iter << ": error = " << current_error << std::endl;
        }
        
        // If no improvement, we've reached a local minimum
        if (!improved) {
            std::cout << "Local minimum reached at iteration " << iter << std::endl;
            break;
        }
    }
    
    std::cout << "Local search complete. Final error: " << current_error << std::endl;
}

void TexelTuner::save_parameters(const std::string& filename) {
    if (!evaluator) return;
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to save parameters to: " << filename << std::endl;
        return;
    }
    
    const auto& params = evaluator->get_params();
    auto param_names = get_parameter_names();
    auto param_ptrs = get_tunable_parameters();
    
    file << "# Texel-tuned evaluation parameters" << std::endl;
    file << "# Error: " << current_error << std::endl;
    file << std::endl;
    
    for (size_t i = 0; i < param_ptrs.size(); i++) {
        file << param_names[i] << " = " << *param_ptrs[i] << std::endl;
    }
    
    file.close();
    std::cout << "Parameters saved to: " << filename << std::endl;
}

namespace tuning {

bool generate_training_data_from_pgn(const std::string& pgn_file,
                                     const std::string& output_file,
                                     int positions_per_game) {
    std::cerr << "PGN parsing not yet implemented" << std::endl;
    std::cerr << "Please provide training data in FEN format:" << std::endl;
    std::cerr << "  <fen> <result>" << std::endl;
    std::cerr << "  Where result is 1.0 (white win), 0.5 (draw), or 0.0 (black win)" << std::endl;
    return false;
}

bool validate_parameters(const ClassicEval::EvalParams& params) {
    // Check for reasonable parameter ranges
    if (params.material_pawn < 50 || params.material_pawn > 150) return false;
    if (params.material_knight < 250 || params.material_knight > 400) return false;
    if (params.material_bishop < 250 || params.material_bishop > 400) return false;
    if (params.material_rook < 400 || params.material_rook > 600) return false;
    if (params.material_queen < 800 || params.material_queen > 1100) return false;
    
    // Check penalties are positive
    if (params.doubled_pawn_penalty < 0 || params.doubled_pawn_penalty > 50) return false;
    if (params.isolated_pawn_penalty < 0 || params.isolated_pawn_penalty > 50) return false;
    if (params.backward_pawn_penalty < 0 || params.backward_pawn_penalty > 50) return false;
    
    // Check passed pawn bonuses are increasing
    for (int i = 2; i < 7; i++) {
        if (params.passed_pawn_bonus[i] > params.passed_pawn_bonus[i + 1]) {
            return false;
        }
    }
    
    return true;
}

std::string export_parameters_json(const ClassicEval::EvalParams& params) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"material\": {\n";
    json << "    \"pawn\": " << params.material_pawn << ",\n";
    json << "    \"knight\": " << params.material_knight << ",\n";
    json << "    \"bishop\": " << params.material_bishop << ",\n";
    json << "    \"rook\": " << params.material_rook << ",\n";
    json << "    \"queen\": " << params.material_queen << "\n";
    json << "  },\n";
    
    json << "  \"pawn_structure\": {\n";
    json << "    \"doubled_penalty\": " << params.doubled_pawn_penalty << ",\n";
    json << "    \"isolated_penalty\": " << params.isolated_pawn_penalty << ",\n";
    json << "    \"backward_penalty\": " << params.backward_pawn_penalty << ",\n";
    json << "    \"chain_bonus\": " << params.pawn_chain_bonus << ",\n";
    json << "    \"passed_bonus\": [";
    for (int i = 0; i < 8; i++) {
        json << params.passed_pawn_bonus[i];
        if (i < 7) json << ", ";
    }
    json << "]\n";
    json << "  },\n";
    
    json << "  \"piece_specific\": {\n";
    json << "    \"rook_open_file\": " << params.rook_open_file_bonus << ",\n";
    json << "    \"rook_semi_open_file\": " << params.rook_semi_open_file_bonus << ",\n";
    json << "    \"bishop_pair\": " << params.bishop_pair_bonus << ",\n";
    json << "    \"knight_outpost\": " << params.knight_outpost_bonus << ",\n";
    json << "    \"bad_bishop\": " << params.bad_bishop_penalty << "\n";
    json << "  },\n";
    
    json << "  \"king_safety\": {\n";
    json << "    \"pawn_shield\": " << params.pawn_shield_bonus << ",\n";
    json << "    \"open_file_penalty\": " << params.open_file_near_king_penalty << ",\n";
    json << "    \"tropism_weight\": " << params.king_tropism_weight << ",\n";
    json << "    \"castling_bonus\": " << params.castling_bonus << ",\n";
    json << "    \"king_center_penalty\": " << params.king_center_penalty << ",\n";
    json << "    \"tropism_uncastled_multiplier\": " << params.king_tropism_uncastled_multiplier << "\n";
    json << "  },\n";
    
    json << "  \"mobility\": {\n";
    json << "    \"knight\": " << params.mobility_weight[KNIGHT] << ",\n";
    json << "    \"bishop\": " << params.mobility_weight[BISHOP] << ",\n";
    json << "    \"rook\": " << params.mobility_weight[ROOK] << ",\n";
    json << "    \"queen\": " << params.mobility_weight[QUEEN] << "\n";
    json << "  },\n";
    
    json << "  \"endgame\": {\n";
    json << "    \"king_activity\": " << params.king_activity_bonus << ",\n";
    json << "    \"opposition\": " << params.opposition_bonus << "\n";
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

bool import_parameters_json(const std::string& json, ClassicEval::EvalParams& params) {
    std::cerr << "JSON import not yet implemented" << std::endl;
    return false;
}

} // namespace tuning

} // namespace chess
