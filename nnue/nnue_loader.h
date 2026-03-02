#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace chess {

// NNUE network loader
// Supports loading weights from binary files
class NnueLoader {
public:
    enum NetworkFormat {
        NATIVE,      // Our custom format
        STOCKFISH    // Stockfish NNUE format
    };
    
    // Auto-detect and load network from file
    static bool load_from_file(const std::string& filename, 
                               int16_t*& input_weights,
                               int16_t*& hidden_bias,
                               int16_t*& output_weights,
                               int16_t*& output_bias,
                               int& input_size,
                               int& hidden_size,
                               int& output_size);
    
    // Load network with explicit format
    static bool load_from_file_format(const std::string& filename,
                                      NetworkFormat format,
                                      int16_t*& input_weights,
                                      int16_t*& hidden_bias,
                                      int16_t*& output_weights,
                                      int16_t*& output_bias,
                                      int& input_size,
                                      int& hidden_size,
                                      int& output_size);
    
    // Detect network format from file
    static NetworkFormat detect_format(const std::string& filename);
    
    // Load native format
    static bool load_native_format(const std::string& filename,
                                   int16_t*& input_weights,
                                   int16_t*& hidden_bias,
                                   int16_t*& output_weights,
                                   int16_t*& output_bias,
                                   int& input_size,
                                   int& hidden_size,
                                   int& output_size);
    
    // Load Stockfish format
    static bool load_stockfish_format(const std::string& filename,
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
    
    // Save network to file (native format)
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

