# Next Steps: Loading NNUE Weights and Implementation

## Overview

The engine now has the framework for NNUE evaluation. Here's how to proceed with the next steps:

## Step 1: Load NNUE Network Weights

### Option A: Use Pre-trained Weights (Recommended)

1. **Get NNUE weights from existing engines:**
   - Stockfish NNUE: Download from [Stockfish releases](https://github.com/official-stockfish/Stockfish/releases)
   - Convert to our format (see below)
   - Or use weights from other open-source engines

2. **Convert weights to our format:**
   - Our format is a simple binary format (see `nnue_loader.h`)
   - You'll need to write a converter script or modify the loader to support other formats

### Option B: Train Your Own Network

1. **Collect training data:**
   - Use games from databases (Lichess, Chess.com, etc.)
   - Extract positions and evaluations
   - Format: (position, evaluation_score)

2. **Train the network:**
   - Use PyTorch/TensorFlow
   - Implement HalfKP feature extraction
   - Train with supervised learning (position -> evaluation)

3. **Export weights:**
   - Quantize to int16_t
   - Save in our binary format

## Step 2: Load Weights in the Engine

### Current Implementation

The engine can now:
- Load weights from binary files via `NnueEval::load_network(filename)`
- Create default random weights for testing
- Use the loaded weights in evaluation

### How to Use

```cpp
// In UCI or initialization code:
NnueEval* nnue = new NnueEval();
if (!nnue->load_network("nnue_weights.bin")) {
    // Fallback to default or classic eval
}
```

### Adding UCI Option

You can add a UCI option to specify the network file:

```cpp
// In uci.cpp handle_setoption:
if (name == "EvalFile") {
    std::string filename;
    iss >> filename;
    if (use_nnue && evaluator) {
        static_cast<NnueEval*>(evaluator)->load_network(filename);
    }
}
```

## Step 3: Improve Forward Pass (SIMD Optimization)

### Current Implementation

The forward pass is implemented but not optimized. To add SIMD:

1. **Use SIMD for matrix multiplication:**
   ```cpp
   #include <immintrin.h>  // AVX2
   
   // Process 8 int16_t values at once
   __m256i acc_vec = _mm256_setzero_si256();
   for (int i = 0; i < hidden_size; i += 8) {
       __m256i weights = _mm256_load_si256(...);
       __m256i hidden = _mm256_load_si256(...);
       acc_vec = _mm256_add_epi16(acc_vec, _mm256_mullo_epi16(weights, hidden));
   }
   ```

2. **Optimize clipped ReLU:**
   - Use SIMD min/max operations
   - Process multiple values in parallel

### Resources

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/)
- Stockfish's NNUE implementation (highly optimized reference)

## Step 4: Test and Validate

### Testing Steps

1. **Load a network:**
   ```bash
   # Create a test network
   ./chess_engine
   # Use UCI to switch to NNUE
   setoption name Use NNUE value true
   ```

2. **Verify evaluation:**
   - Compare NNUE vs ClassicEval on same positions
   - Check that scores are reasonable (centipawns)
   - Verify incremental updates work (make/unmake)

3. **Performance testing:**
   - Measure evaluation speed
   - Compare with/without SIMD
   - Profile with tools like `perf` or `vtune`

### Expected Behavior

- NNUE should give more accurate evaluations than ClassicEval
- Evaluation should be fast (< 1ms per position)
- Incremental updates should be faster than full recomputation

## Step 5: Advanced Optimizations

### Accumulator Optimization

Currently, we refresh on unmake. You can optimize by:
- Storing accumulator history in UndoInfo
- Reversing delta operations instead of refreshing

### Feature Index Optimization

- Precompute feature indices for common patterns
- Cache feature lookups
- Use lookup tables for king-dependent features

### Network Architecture

- Experiment with different hidden layer sizes
- Try multiple hidden layers
- Add feature engineering (piece-square combinations)

## Getting Started Right Now

### Quick Test with Default Network

1. Build the engine:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

2. Run and test:
   ```bash
   ./chess_engine
   # In UCI:
   setoption name Use NNUE value true
   position startpos
   go depth 5
   ```

The default network uses random weights, so it won't play well, but it will verify the infrastructure works.

### Creating a Test Network File

You can create a test network file programmatically:

```cpp
// In a test program or UCI command:
NnueEval nnue;
nnue.create_default_network();
// Save it:
NnueLoader::save_to_file("test_network.bin", ...);
```

## Resources

- **Stockfish NNUE**: [GitHub](https://github.com/official-stockfish/Stockfish) - Reference implementation
- **NNUE Paper**: "Efficiently Updatable Neural-Network-based Evaluation Functions" by Yu Nasu
- **HalfKP Explanation**: See Stockfish's documentation on feature sets

## Next: Advanced Search Techniques

Once NNUE is working:
1. Add razoring (reduce depth in quiet positions)
2. Add futility pruning (skip moves unlikely to improve alpha)
3. Improve move ordering (history heuristic, counter moves)
4. Add time management improvements
5. Tune search parameters

