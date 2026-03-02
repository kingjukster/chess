# NNUE Network Format Specification

This document describes the NNUE (Efficiently Updatable Neural Network) formats supported by this chess engine.

## Supported Formats

### 1. Native Format

Our custom binary format optimized for simplicity and portability.

#### File Structure

```
[Header]
- version (uint32_t): Format version (currently 1)
- input_size (uint32_t): Number of input features
- hidden_size (uint32_t): Hidden layer size
- output_size (uint32_t): Output layer size (typically 1)
- checksum (uint32_t): Simple checksum for validation

[Feature Transformer Weights]
- input_weights (int16_t[input_size * hidden_size]): Input layer weights
- hidden_bias (int16_t[hidden_size]): Hidden layer biases

[Output Layer]
- output_weights (int16_t[hidden_size * output_size]): Output layer weights
- output_bias (int16_t[output_size]): Output layer bias
```

#### Quantization

- All weights are stored as 16-bit signed integers (int16_t)
- Typical scale factor: 64 (i.e., weight = float_value * 64)
- Biases use the same scale factor

#### Example Dimensions

For HalfKP architecture:
- input_size: 40960 (64 king squares × 640 piece features)
- hidden_size: 256 (typical)
- output_size: 1

### 2. Stockfish Format

Compatible with Stockfish NNUE networks (*.nnue files).

#### File Structure

```
[Header]
- version (uint32_t): NNUE version (0x7AF32F16 or 0x7AF32F17)
- hash (uint32_t): Network hash for identification
- architecture_length (uint32_t): Length of architecture string
- architecture (char[]): Architecture description (e.g., "HalfKP(Friend)[41024->256x2]->1")

[Feature Transformer]
- ft_weights (int16_t[41024 * 256]): Feature transformer weights
- ft_biases (int16_t[256]): Feature transformer biases

[Output Layer]
- output_weights (int8_t[256 * 2]): Output weights for both perspectives
- output_bias (int32_t): Output bias
```

#### Key Differences from Native Format

1. **Version Magic**: Stockfish uses 0x7AF32F16 instead of version 1
2. **Architecture String**: Includes human-readable architecture description
3. **Output Weights**: Uses int8_t instead of int16_t
4. **Output Bias**: Uses int32_t instead of int16_t
5. **Dual Perspective**: Output layer has weights for both white and black perspectives

#### Conversion Notes

When converting from Stockfish format to native:
- Output weights are cast from int8_t to int16_t
- Output bias is scaled down (divided by 16)
- Only one perspective is used (white's perspective)

## Feature Set: HalfKP

Both formats use the HalfKP (Half King-Piece) feature set.

### Feature Encoding

Each position is encoded relative to the king position:

```
feature_index = king_square * 640 + piece_index
```

Where `piece_index` is calculated as:
```
piece_index = piece_square * 10 + piece_type * 2 + piece_color
```

### Piece Type Encoding

- PAWN: 0
- KNIGHT: 1
- BISHOP: 2
- ROOK: 3
- QUEEN: 4
- (KING is not included in features)

### Color Encoding

- WHITE: 0
- BLACK: 1

### Total Features

- 64 king squares
- 64 piece squares × 5 piece types × 2 colors = 640 piece features
- Total: 64 × 640 = 40,960 features

## Network Architecture

### Standard Architecture (256x2->1)

```
Input Layer (40960 features)
    ↓
Feature Transformer (40960 → 256)
    ↓
Accumulator (256 values per perspective)
    ↓
Clipped ReLU Activation
    ↓
Output Layer (256 → 1)
    ↓
Evaluation Score (centipawns)
```

### Activation Functions

- **Hidden Layer**: Clipped ReLU
  - `output = max(0, min(127, input))`
- **Output Layer**: Linear (no activation)

### Incremental Updates

The feature transformer accumulator can be updated incrementally:

1. **On piece move**: Subtract old feature weights, add new feature weights
2. **On capture**: Subtract captured piece feature weights
3. **On promotion**: Subtract pawn weights, add promoted piece weights
4. **On castling**: Update both king and rook features

This allows O(1) evaluation updates instead of O(N) full recalculation.

## Loading Networks

### Auto-Detection

The loader automatically detects the format by reading the first 4 bytes:
- If magic == 0x7AF32F16 or 0x7AF32F17: Stockfish format
- Otherwise: Native format

### Example Usage

```cpp
#include "nnue/nnue_eval.h"

NnueEval evaluator;

// Auto-detect and load
evaluator.load_network("network.nnue");  // Stockfish format
evaluator.load_network("network.bin");   // Native format
```

## Creating Networks

### Training

Networks are typically trained using:
1. Large dataset of positions with known evaluations
2. Supervised learning (regression to target evaluation)
3. Gradient descent optimization (Adam, SGD, etc.)

### Tools

- **Stockfish Training**: Use Stockfish's trainer with game databases
- **Custom Training**: Implement your own trainer using the native format
- **Texel Tuning**: Fine-tune evaluation parameters (see `eval/tuner.h`)

## Converting Networks

### Stockfish to Native

```cpp
#include "nnue/stockfish_nnue.h"
#include "nnue/nnue_loader.h"

// Load Stockfish network
StockfishNnueLoader::StockfishNetwork sf_network;
StockfishNnueLoader::load("stockfish.nnue", sf_network);

// Convert to native format
int16_t *input_weights, *hidden_bias, *output_weights, *output_bias;
int input_size, hidden_size, output_size;

StockfishNnueLoader::convert_to_native(
    sf_network, input_weights, hidden_bias, output_weights, output_bias,
    input_size, hidden_size, output_size
);

// Save as native format
NnueLoader::save_to_file("network.bin", input_weights, hidden_bias,
                        output_weights, output_bias, input_size,
                        hidden_size, output_size);
```

## Performance Considerations

### Memory Usage

- **Native Format**: ~20-40 MB for typical 256-hidden network
- **Stockfish Format**: Similar, slightly larger due to dual perspective

### Inference Speed

- **Full Evaluation**: ~100-500 ns per position (with warm cache)
- **Incremental Update**: ~10-50 ns per move
- **Speedup**: 10-50x faster with incremental updates

### Optimization Tips

1. **SIMD**: Use AVX2/AVX-512 for accumulator updates
2. **Cache**: Keep accumulators in CPU cache
3. **Lazy Evaluation**: Only refresh dirty accumulators
4. **Batch Processing**: Process multiple positions together

## References

- [Stockfish NNUE](https://github.com/official-stockfish/Stockfish/blob/master/src/nnue/)
- [NNUE Paper](https://arxiv.org/abs/2109.13579)
- [Efficiently Updatable Neural Networks](https://github.com/ynasu87/nnue-pytorch)

## UCI Commands

### Loading Networks

```
setoption name Use NNUE value true
setoption name EvalFile value network.nnue
```

### Exporting Parameters

```
exportparams eval_params.json
```

### Tuning Mode

```
setoption name Tuning Mode value true
setoption name Training Data value training.epd
tune iterations 1000 lr 1.0
```
