#include "stockfish_nnue.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace chess {

template<typename T>
bool StockfishNnueLoader::read_value(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return !file.fail();
}

bool StockfishNnueLoader::read_string(std::ifstream& file, std::string& str) {
    uint32_t length;
    if (!read_value(file, length)) return false;
    
    if (length > 1024) return false;
    
    str.resize(length);
    file.read(&str[0], length);
    return !file.fail();
}

template<typename T>
bool StockfishNnueLoader::read_array(std::ifstream& file, T*& array, size_t count) {
    array = new T[count];
    file.read(reinterpret_cast<char*>(array), count * sizeof(T));
    return !file.fail();
}

bool StockfishNnueLoader::load(const std::string& filename, StockfishNetwork& network) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open Stockfish NNUE file: " << filename << std::endl;
        return false;
    }
    
    // Read version
    if (!read_value(file, network.version)) {
        std::cerr << "Failed to read NNUE version" << std::endl;
        return false;
    }
    
    // Verify version (support 0x7AF32F16 and 0x7AF32F17)
    if (network.version != NNUE_VERSION && network.version != NNUE_VERSION_ALT) {
        std::cerr << "Unsupported NNUE version: 0x" << std::hex << network.version << std::dec << std::endl;
        std::cerr << "Expected: 0x" << std::hex << NNUE_VERSION << " or 0x" << NNUE_VERSION_ALT << std::dec << std::endl;
        return false;
    }
    
    // Read hash
    if (!read_value(file, network.hash)) {
        std::cerr << "Failed to read NNUE hash" << std::endl;
        return false;
    }
    
    // Read architecture string
    if (!read_string(file, network.architecture)) {
        std::cerr << "Failed to read architecture string" << std::endl;
        return false;
    }
    
    std::cout << "Loading Stockfish NNUE: " << network.architecture << std::endl;
    std::cout << "Version: 0x" << std::hex << network.version << std::dec << std::endl;
    std::cout << "Hash: 0x" << std::hex << network.hash << std::dec << std::endl;
    
    // Read feature transformer weights (HalfKP -> 256)
    network.input_size = HALFKP_FEATURES;
    network.hidden_size = HIDDEN_SIZE;
    
    size_t ft_weight_count = network.input_size * network.hidden_size;
    if (!read_array(file, network.ft_weights, ft_weight_count)) {
        std::cerr << "Failed to read feature transformer weights" << std::endl;
        return false;
    }
    
    // Read feature transformer biases
    if (!read_array(file, network.ft_biases, network.hidden_size)) {
        std::cerr << "Failed to read feature transformer biases" << std::endl;
        return false;
    }
    
    // Read output layer weights (256x2 -> 1)
    size_t output_weight_count = network.hidden_size * 2;
    if (!read_array(file, network.output_weights, output_weight_count)) {
        std::cerr << "Failed to read output weights" << std::endl;
        return false;
    }
    
    // Read output bias
    if (!read_value(file, network.output_bias)) {
        std::cerr << "Failed to read output bias" << std::endl;
        return false;
    }
    
    file.close();
    
    std::cout << "Successfully loaded Stockfish NNUE network" << std::endl;
    std::cout << "  Input features: " << network.input_size << std::endl;
    std::cout << "  Hidden size: " << network.hidden_size << std::endl;
    
    return true;
}

bool StockfishNnueLoader::verify_network(const StockfishNetwork& network) {
    if (network.ft_weights == nullptr || network.ft_biases == nullptr || 
        network.output_weights == nullptr) {
        return false;
    }
    
    if (network.input_size != HALFKP_FEATURES) {
        std::cerr << "Invalid input size: " << network.input_size << std::endl;
        return false;
    }
    
    if (network.hidden_size != HIDDEN_SIZE) {
        std::cerr << "Invalid hidden size: " << network.hidden_size << std::endl;
        return false;
    }
    
    return true;
}

bool StockfishNnueLoader::convert_to_native(const StockfishNetwork& sf_network,
                                            int16_t*& input_weights,
                                            int16_t*& hidden_bias,
                                            int16_t*& output_weights,
                                            int16_t*& output_bias,
                                            int& input_size,
                                            int& hidden_size,
                                            int& output_size) {
    if (!verify_network(sf_network)) {
        return false;
    }
    
    input_size = sf_network.input_size;
    hidden_size = sf_network.hidden_size;
    output_size = 1;
    
    // Allocate native format arrays
    size_t ft_weight_count = input_size * hidden_size;
    input_weights = new int16_t[ft_weight_count];
    hidden_bias = new int16_t[hidden_size];
    output_weights = new int16_t[hidden_size * output_size];
    output_bias = new int16_t[output_size];
    
    // Copy feature transformer weights
    std::memcpy(input_weights, sf_network.ft_weights, ft_weight_count * sizeof(int16_t));
    
    // Copy feature transformer biases
    std::memcpy(hidden_bias, sf_network.ft_biases, hidden_size * sizeof(int16_t));
    
    // Convert output layer (Stockfish uses int8_t, we use int16_t)
    // Stockfish has 256x2 weights (both perspectives), we'll use just one perspective
    for (int i = 0; i < hidden_size; i++) {
        output_weights[i] = static_cast<int16_t>(sf_network.output_weights[i]);
    }
    
    // Convert output bias (int32_t to int16_t)
    output_bias[0] = static_cast<int16_t>(sf_network.output_bias / 16);
    
    std::cout << "Converted Stockfish NNUE to native format" << std::endl;
    return true;
}

} // namespace chess
