#!/usr/bin/env python3
"""
Export trained PyTorch model to engine's binary format.
Converts float32 weights to int16_t quantized weights.
"""

import torch
import struct
import argparse
import numpy as np

FEATURES_PER_KING = 64 * 5 * 2  # 640
HIDDEN_SIZE = 256
OUTPUT_SIZE = 1

def quantize_weights(weights, scale_factor=64):
    """Quantize float32 weights to int16_t."""
    # Scale and clamp to int16 range
    quantized = (weights * scale_factor).round().clamp(-32768, 32767)
    return quantized.cpu().numpy().astype(np.int16)

def export_model(model_path, output_path, quantize_scale=64):
    """Export PyTorch model to engine format."""
    print(f"Loading model from {model_path}...")
    
    # Define network architecture (same as train_nnue.py)
    import torch.nn as nn
    
    class NnueNetwork(nn.Module):
        def __init__(self, input_size=FEATURES_PER_KING, hidden_size=HIDDEN_SIZE, output_size=OUTPUT_SIZE):
            super(NnueNetwork, self).__init__()
            self.input_layer = nn.Linear(input_size, hidden_size, bias=True)
            self.output_layer = nn.Linear(hidden_size, output_size, bias=True)
    
    # Load model
    model = NnueNetwork()
    model.load_state_dict(torch.load(model_path, map_location='cpu'))
    model.eval()
    
    print("Model loaded. Extracting weights...")
    
    # Extract weights
    input_weights = model.input_layer.weight.data  # [hidden_size, input_size]
    hidden_bias = model.input_layer.bias.data      # [hidden_size]
    output_weights = model.output_layer.weight.data  # [output_size, hidden_size]
    output_bias = model.output_layer.bias.data     # [output_size]
    
    # Transpose input weights to [input_size, hidden_size] (row-major)
    input_weights = input_weights.t()
    
    # Quantize
    print("Quantizing weights...")
    input_weights_q = quantize_weights(input_weights, quantize_scale)
    hidden_bias_q = quantize_weights(hidden_bias, quantize_scale)
    output_weights_q = quantize_weights(output_weights, quantize_scale)
    output_bias_q = quantize_weights(output_bias, quantize_scale)
    
    # Write to binary file
    print(f"Writing to {output_path}...")
    with open(output_path, 'wb') as f:
        # Write header
        header = struct.pack('IIII', 
            1,  # version
            FEATURES_PER_KING,  # input_size
            HIDDEN_SIZE,  # hidden_size
            OUTPUT_SIZE)  # output_size
        f.write(header)
        f.write(struct.pack('I', 0))  # checksum (placeholder)
        
        # Write weights
        f.write(input_weights_q.tobytes())
        f.write(hidden_bias_q.tobytes())
        f.write(output_weights_q.tobytes())
        f.write(output_bias_q.tobytes())
    
    print("Export complete!")
    print(f"  Input weights: {input_weights_q.shape}")
    print(f"  Hidden bias: {hidden_bias_q.shape}")
    print(f"  Output weights: {output_weights_q.shape}")
    print(f"  Output bias: {output_bias_q.shape}")
    
    # Calculate file size
    file_size = (16 +  # header
                 FEATURES_PER_KING * HIDDEN_SIZE * 2 +  # input weights
                 HIDDEN_SIZE * 2 +  # hidden bias
                 HIDDEN_SIZE * OUTPUT_SIZE * 2 +  # output weights
                 OUTPUT_SIZE * 2)  # output bias
    
    print(f"  File size: {file_size} bytes ({file_size / 1024:.2f} KB)")

def main():
    parser = argparse.ArgumentParser(description='Export trained model to engine format')
    parser.add_argument('--model', required=True, help='PyTorch model file (.pth)')
    parser.add_argument('--output', required=True, help='Output binary file')
    parser.add_argument('--scale', type=int, default=64, help='Quantization scale factor')
    
    args = parser.parse_args()
    
    export_model(args.model, args.output, args.scale)

if __name__ == '__main__':
    main()

