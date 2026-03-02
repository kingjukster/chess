#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace chess {

// Stockfish NNUE format loader
// Supports loading Stockfish NNUE networks (*.nnue files)
class StockfishNnueLoader {
public:
    // Stockfish NNUE network structure
    struct StockfishNetwork {
        // Architecture info
        uint32_t version;
        uint32_t hash;
        std::string architecture;
        
        // Feature transformer (HalfKP -> 256x2)
        int16_t* ft_weights;      // [41024 * 256] input features to hidden
        int16_t* ft_biases;       // [256] hidden layer biases
        
        // Output layer (256x2 -> 1)
        int8_t* output_weights;   // [256 * 2] (both perspectives)
        int32_t output_bias;
        
        // Dimensions
        int input_size;
        int hidden_size;
        
        StockfishNetwork() : version(0), hash(0), ft_weights(nullptr), 
                            ft_biases(nullptr), output_weights(nullptr),
                            output_bias(0), input_size(0), hidden_size(0) {}
        
        void cleanup() {
            delete[] ft_weights;
            delete[] ft_biases;
            delete[] output_weights;
            ft_weights = nullptr;
            ft_biases = nullptr;
            output_weights = nullptr;
        }
    };
    
    // Load Stockfish NNUE network from file
    static bool load(const std::string& filename, StockfishNetwork& network);
    
    // Convert Stockfish network to our format
    static bool convert_to_native(const StockfishNetwork& sf_network,
                                   int16_t*& input_weights,
                                   int16_t*& hidden_bias,
                                   int16_t*& output_weights,
                                   int16_t*& output_bias,
                                   int& input_size,
                                   int& hidden_size,
                                   int& output_size);
    
    // Verify network integrity
    static bool verify_network(const StockfishNetwork& network);

private:
    // Stockfish NNUE file format constants (support multiple versions)
    static constexpr uint32_t NNUE_VERSION = 0x7AF32F16;
    static constexpr uint32_t NNUE_VERSION_ALT = 0x7AF32F17;
    static constexpr int HALFKP_FEATURES = 41024;  // 64 king squares * 641 piece features
    static constexpr int HIDDEN_SIZE = 256;
    
    // Helper functions for reading binary data
    template<typename T>
    static bool read_value(std::ifstream& file, T& value);
    
    static bool read_string(std::ifstream& file, std::string& str);
    
    template<typename T>
    static bool read_array(std::ifstream& file, T*& array, size_t count);
};

} // namespace chess
