#include "nnue_loader.h"
#include "stockfish_nnue.h"
#include <iostream>
#include <string>

// Standalone utility to convert NNUE networks between formats
// Usage: nnue_converter <input_file> <output_file> [format]
//   format: native, stockfish (default: auto-detect input, output as native)

using namespace chess;

void print_usage() {
    std::cout << "NNUE Network Converter" << std::endl;
    std::cout << "Usage: nnue_converter <input_file> <output_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported formats:" << std::endl;
    std::cout << "  - Stockfish NNUE (*.nnue)" << std::endl;
    std::cout << "  - Native format (*.bin)" << std::endl;
    std::cout << std::endl;
    std::cout << "The converter auto-detects the input format and converts to native format." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    
    std::cout << "Converting: " << input_file << " -> " << output_file << std::endl;
    
    // Load network (auto-detect format)
    int16_t *input_weights = nullptr, *hidden_bias = nullptr;
    int16_t *output_weights = nullptr, *output_bias = nullptr;
    int input_size, hidden_size, output_size;
    
    if (!NnueLoader::load_from_file(input_file, input_weights, hidden_bias,
                                    output_weights, output_bias, input_size,
                                    hidden_size, output_size)) {
        std::cerr << "Failed to load input network" << std::endl;
        return 1;
    }
    
    std::cout << "Loaded network:" << std::endl;
    std::cout << "  Input size: " << input_size << std::endl;
    std::cout << "  Hidden size: " << hidden_size << std::endl;
    std::cout << "  Output size: " << output_size << std::endl;
    
    // Save in native format
    if (!NnueLoader::save_to_file(output_file, input_weights, hidden_bias,
                                  output_weights, output_bias, input_size,
                                  hidden_size, output_size)) {
        std::cerr << "Failed to save output network" << std::endl;
        delete[] input_weights;
        delete[] hidden_bias;
        delete[] output_weights;
        delete[] output_bias;
        return 1;
    }
    
    std::cout << "Successfully converted to: " << output_file << std::endl;
    
    // Cleanup
    delete[] input_weights;
    delete[] hidden_bias;
    delete[] output_weights;
    delete[] output_bias;
    
    return 0;
}
