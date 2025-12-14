#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace chess {

// NNUE network loader
// Supports loading weights from binary files
class NnueLoader {
public:
    // Load network from file
    // Format: binary file with:
    // - Header (version, input_size, hidden_size, output_size)
    // - Input layer weights (input_size * hidden_size * sizeof(int16_t))
    // - Hidden layer bias (hidden_size * sizeof(int16_t))
    // - Output layer weights (hidden_size * output_size * sizeof(int16_t))
    // - Output layer bias (output_size * sizeof(int16_t))
    static bool load_from_file(const std::string& filename, 
                               int16_t*& input_weights,
                               int16_t*& hidden_bias,
                               int16_t*& output_weights,
                               int16_t*& output_bias,
                               int& input_size,
                               int& hidden_size,
                               int& output_size);
    
    // Create a default/random network (for testing)
    static void create_default_network(int16_t*& input_weights,
                                       int16_t*& hidden_bias,
                                       int16_t*& output_weights,
                                       int16_t*& output_bias,
                                       int input_size,
                                       int hidden_size,
                                       int output_size);
    
    // Save network to file
    static bool save_to_file(const std::string& filename,
                             const int16_t* input_weights,
                             const int16_t* hidden_bias,
                             const int16_t* output_weights,
                             const int16_t* output_bias,
                             int input_size,
                             int hidden_size,
                             int output_size);
};

} // namespace chess

