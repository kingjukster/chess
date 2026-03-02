#pragma once

#include "classic_eval.h"
#include "../board/position.h"
#include <vector>
#include <string>
#include <functional>

namespace chess {

// Texel tuning implementation for optimizing evaluation parameters
// Based on the method described by Peter Österlund
class TexelTuner {
public:
    TexelTuner();
    ~TexelTuner() = default;
    
    // Load training data from file
    // Format: FEN result (1.0 = white win, 0.5 = draw, 0.0 = black win)
    bool load_training_data(const std::string& filename);
    
    // Run tuning with gradient descent
    void tune(int iterations = 1000, double learning_rate = 1.0);
    
    // Save tuned parameters
    void save_parameters(const std::string& filename);
    
    // Get/set evaluator
    void set_evaluator(ClassicEval* eval) { evaluator = eval; }
    ClassicEval* get_evaluator() { return evaluator; }
    
    // Get tuning statistics
    double get_error() const { return current_error; }
    int get_position_count() const { return static_cast<int>(positions.size()); }

private:
    struct TrainingPosition {
        Position pos;
        double result;  // 1.0 = white win, 0.5 = draw, 0.0 = black win
    };
    
    std::vector<TrainingPosition> positions;
    ClassicEval* evaluator;
    double current_error;
    
    // Error calculation (mean squared error)
    double calculate_error();
    
    // Sigmoid function for converting centipawn score to win probability
    double sigmoid(double score, double k = 1.0) const;
    
    // Parameter extraction and setting
    std::vector<int*> get_tunable_parameters();
    std::vector<std::string> get_parameter_names();
    
    // Gradient calculation
    void calculate_gradients(std::vector<double>& gradients);
    
    // Local search optimization (optional, more robust than pure gradient descent)
    void local_search(int iterations);
};

// Utility functions for tuning
namespace tuning {
    // Generate training data from PGN files
    bool generate_training_data_from_pgn(const std::string& pgn_file, 
                                         const std::string& output_file,
                                         int positions_per_game = 5);
    
    // Validate tuned parameters (check for reasonable values)
    bool validate_parameters(const ClassicEval::EvalParams& params);
    
    // Export parameters to JSON format
    std::string export_parameters_json(const ClassicEval::EvalParams& params);
    
    // Import parameters from JSON format
    bool import_parameters_json(const std::string& json, ClassicEval::EvalParams& params);
}

} // namespace chess
