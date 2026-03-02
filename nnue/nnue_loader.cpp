#include "nnue_loader.h"
#include "stockfish_nnue.h"
#include <cstring>
#include <random>
#include <iostream>

namespace chess {

struct NetworkHeader {
    uint32_t version;
    uint32_t input_size;
    uint32_t hidden_size;
    uint32_t output_size;
    uint32_t checksum;
};

NnueLoader::NetworkFormat NnueLoader::detect_format(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return NATIVE;
    }
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.close();
    
    // Stockfish NNUE magic number
    if (magic == 0x7AF32F16 || magic == 0x7AF32F17) {
        return STOCKFISH;
    }
    
    return NATIVE;
}

bool NnueLoader::load_from_file(const std::string& filename,
                                int16_t*& input_weights,
                                int16_t*& hidden_bias,
                                int16_t*& output_weights,
                                int16_t*& output_bias,
                                int& input_size,
                                int& hidden_size,
                                int& output_size) {
    NetworkFormat format = detect_format(filename);
    
    std::cout << "Detected network format: " 
              << (format == STOCKFISH ? "Stockfish" : "Native") << std::endl;
    
    return load_from_file_format(filename, format, input_weights, hidden_bias,
                                 output_weights, output_bias, input_size, 
                                 hidden_size, output_size);
}

bool NnueLoader::load_from_file_format(const std::string& filename,
                                       NetworkFormat format,
                                       int16_t*& input_weights,
                                       int16_t*& hidden_bias,
                                       int16_t*& output_weights,
                                       int16_t*& output_bias,
                                       int& input_size,
                                       int& hidden_size,
                                       int& output_size) {
    if (format == STOCKFISH) {
        return load_stockfish_format(filename, input_weights, hidden_bias,
                                    output_weights, output_bias, input_size,
                                    hidden_size, output_size);
    } else {
        return load_native_format(filename, input_weights, hidden_bias,
                                 output_weights, output_bias, input_size,
                                 hidden_size, output_size);
    }
}

bool NnueLoader::load_native_format(const std::string& filename,
                                    int16_t*& input_weights,
                                    int16_t*& hidden_bias,
                                    int16_t*& output_weights,
                                    int16_t*& output_bias,
                                    int& input_size,
                                    int& hidden_size,
                                    int& output_size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open network file: " << filename << std::endl;
        return false;
    }
    
    NetworkHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.version != 1) {
        std::cerr << "Unsupported network version: " << header.version << std::endl;
        return false;
    }
    
    input_size = header.input_size;
    hidden_size = header.hidden_size;
    output_size = header.output_size;
    
    input_weights = new int16_t[input_size * hidden_size];
    hidden_bias = new int16_t[hidden_size];
    output_weights = new int16_t[hidden_size * output_size];
    output_bias = new int16_t[output_size];
    
    file.read(reinterpret_cast<char*>(input_weights), input_size * hidden_size * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(hidden_bias), hidden_size * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(output_weights), hidden_size * output_size * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(output_bias), output_size * sizeof(int16_t));
    
    if (file.fail()) {
        std::cerr << "Error reading network weights" << std::endl;
        delete[] input_weights;
        delete[] hidden_bias;
        delete[] output_weights;
        delete[] output_bias;
        input_weights = hidden_bias = output_weights = output_bias = nullptr;
        return false;
    }
    
    file.close();
    std::cout << "Loaded native NNUE network" << std::endl;
    return true;
}

bool NnueLoader::load_stockfish_format(const std::string& filename,
                                       int16_t*& input_weights,
                                       int16_t*& hidden_bias,
                                       int16_t*& output_weights,
                                       int16_t*& output_bias,
                                       int& input_size,
                                       int& hidden_size,
                                       int& output_size) {
    StockfishNnueLoader::StockfishNetwork sf_network;
    
    if (!StockfishNnueLoader::load(filename, sf_network)) {
        return false;
    }
    
    bool success = StockfishNnueLoader::convert_to_native(
        sf_network, input_weights, hidden_bias, output_weights, output_bias,
        input_size, hidden_size, output_size
    );
    
    sf_network.cleanup();
    return success;
}

bool NnueLoader::save_to_file(const std::string& filename,
                              const int16_t* input_weights,
                              const int16_t* hidden_bias,
                              const int16_t* output_weights,
                              const int16_t* output_bias,
                              int input_size,
                              int hidden_size,
                              int output_size) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create network file: " << filename << std::endl;
        return false;
    }
    
    NetworkHeader header;
    header.version = 1;
    header.input_size = input_size;
    header.hidden_size = hidden_size;
    header.output_size = output_size;
    header.checksum = 0; // TODO: Calculate checksum
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(input_weights), input_size * hidden_size * sizeof(int16_t));
    file.write(reinterpret_cast<const char*>(hidden_bias), hidden_size * sizeof(int16_t));
    file.write(reinterpret_cast<const char*>(output_weights), hidden_size * output_size * sizeof(int16_t));
    file.write(reinterpret_cast<const char*>(output_bias), output_size * sizeof(int16_t));
    
    file.close();
    return !file.fail();
}

void NnueLoader::create_default_network(int16_t*& input_weights,
                                        int16_t*& hidden_bias,
                                        int16_t*& output_weights,
                                        int16_t*& output_bias,
                                        int input_size,
                                        int hidden_size,
                                        int output_size) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int16_t> weight_dist(-100, 100);
    std::uniform_int_distribution<int16_t> bias_dist(-50, 50);
    
    // Allocate and initialize with small random values
    input_weights = new int16_t[input_size * hidden_size];
    for (int i = 0; i < input_size * hidden_size; i++) {
        input_weights[i] = weight_dist(rng);
    }
    
    hidden_bias = new int16_t[hidden_size];
    for (int i = 0; i < hidden_size; i++) {
        hidden_bias[i] = bias_dist(rng);
    }
    
    output_weights = new int16_t[hidden_size * output_size];
    for (int i = 0; i < hidden_size * output_size; i++) {
        output_weights[i] = weight_dist(rng);
    }
    
    output_bias = new int16_t[output_size];
    for (int i = 0; i < output_size; i++) {
        output_bias[i] = bias_dist(rng);
    }
}

} // namespace chess

