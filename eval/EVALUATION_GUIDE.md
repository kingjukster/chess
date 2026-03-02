# Chess Engine Evaluation System

This document describes the evaluation system used in this chess engine, including classic evaluation, NNUE integration, and Texel tuning.

## Overview

The engine supports two evaluation methods:

1. **Classic Evaluation**: Hand-crafted evaluation function with tunable parameters
2. **NNUE Evaluation**: Neural network-based evaluation (Efficiently Updatable Neural Network)

## Classic Evaluation

### Components

The classic evaluator (`classic_eval.cpp`) implements a comprehensive evaluation function with the following components:

#### 1. Material and Piece-Square Tables
- Base material values for all pieces
- Position-dependent bonuses via piece-square tables
- Tapered evaluation between middlegame and endgame

#### 2. Pawn Structure (100-200 centipawns impact)
- **Doubled Pawns**: Penalty for pawns on the same file (-10 cp)
- **Isolated Pawns**: Penalty for pawns with no friendly pawns on adjacent files (-15 cp)
- **Passed Pawns**: Bonus for pawns with no enemy pawns blocking their path (10-200 cp by rank)
- **Backward Pawns**: Penalty for pawns that cannot advance safely (-8 cp)
- **Pawn Chains**: Bonus for pawns supporting each other diagonally (+5 cp)

#### 3. King Safety (50-150 centipawns impact in middlegame)
- **Pawn Shield**: Bonus for pawns protecting the king (+10 cp per pawn)
- **Open Files Near King**: Penalty for open files adjacent to king (-20 cp per file)
- **King Tropism**: Penalty based on enemy piece proximity to king (-5 cp per distance unit)

#### 4. Piece Mobility (50-100 centipawns impact)
- Knights: +4 cp per legal move
- Bishops: +3 cp per legal move
- Rooks: +2 cp per legal move
- Queens: +1 cp per legal move

#### 5. Piece-Specific Features
- **Rook on Open File**: +25 cp
- **Rook on Semi-Open File**: +12 cp
- **Bishop Pair**: +30 cp
- **Knight Outpost**: +15 cp (protected square immune to pawn attacks)
- **Bad Bishop**: -20 cp (bishop blocked by own pawns on same color)

#### 6. Endgame Features
- **King Activity**: +10 cp per square closer to center
- **Opposition**: +20 cp in pawn endgames
- **Zugzwang Detection**: (implicit in search, not evaluation)

### Game Phase Detection

The engine uses tapered evaluation to smoothly transition between middlegame and endgame:

```cpp
phase = (knight_count + bishop_count + 2*rook_count + 4*queen_count) * 256 / 24
score = (mg_score * phase + eg_score * (256 - phase)) / 256
```

- Phase 256: Opening/Middlegame (full material)
- Phase 128: Transition
- Phase 0: Endgame (kings and pawns only)

### Tunable Parameters

All evaluation parameters can be tuned using Texel tuning:

```cpp
struct EvalParams {
    // Material values
    int material_pawn;      // Default: 100
    int material_knight;    // Default: 320
    int material_bishop;    // Default: 330
    int material_rook;      // Default: 500
    int material_queen;     // Default: 900
    
    // Pawn structure
    int doubled_pawn_penalty;    // Default: 10
    int isolated_pawn_penalty;   // Default: 15
    int backward_pawn_penalty;   // Default: 8
    int passed_pawn_bonus[8];    // Default: 0,0,10,20,35,60,100,200
    int pawn_chain_bonus;        // Default: 5
    
    // Piece-specific
    int rook_open_file_bonus;       // Default: 25
    int rook_semi_open_file_bonus;  // Default: 12
    int bishop_pair_bonus;          // Default: 30
    int knight_outpost_bonus;       // Default: 15
    int bad_bishop_penalty;         // Default: 20
    
    // King safety
    int pawn_shield_bonus;           // Default: 10
    int open_file_near_king_penalty; // Default: 20
    int king_tropism_weight;         // Default: 5
    
    // Mobility
    int mobility_weight[6];  // Default: 0,0,4,3,2,1
    
    // Endgame
    int king_activity_bonus;  // Default: 10
    int opposition_bonus;     // Default: 20
};
```

## NNUE Evaluation

### Architecture

The NNUE evaluator uses the HalfKP feature set:
- **Input**: 40,960 features (64 king positions × 640 piece features)
- **Hidden Layer**: 256 neurons with clipped ReLU activation
- **Output**: 1 neuron (evaluation in centipawns)

### Incremental Updates

NNUE maintains an accumulator that is updated incrementally:
- **O(1) updates** on piece moves (vs O(N) full evaluation)
- **10-50x faster** than full network evaluation
- Accumulator tracks active features for each king position

### Loading Networks

```bash
# UCI commands
setoption name Use NNUE value true
setoption name EvalFile value network.nnue
```

Supported formats:
- Stockfish NNUE format (*.nnue)
- Native binary format (*.bin)

See `nnue/NNUE_FORMAT.md` for detailed format specifications.

## Texel Tuning

### Overview

Texel tuning is an automated method for optimizing evaluation parameters using a large dataset of positions with known outcomes.

### Usage

1. **Prepare Training Data**

Create a file with positions in EPD format:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 1.0
r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1 0.5
...
```

Format: `<FEN> <result>`
- Result: 1.0 = white win, 0.5 = draw, 0.0 = black win

2. **Enable Tuning Mode**

```bash
setoption name Tuning Mode value true
setoption name Training Data value training.epd
```

3. **Run Tuning**

```bash
tune iterations 1000 lr 1.0
```

Parameters:
- `iterations`: Number of tuning iterations (default: 1000)
- `lr`: Learning rate (default: 1.0)

4. **Export Tuned Parameters**

```bash
exportparams eval_params.json
```

### Algorithm

The tuner uses gradient descent to minimize mean squared error:

```
error = Σ(sigmoid(eval(pos)) - result)² / N
```

Where `sigmoid(x) = 1 / (1 + e^(-x/400))` converts centipawn scores to win probabilities.

### Best Practices

1. **Training Data Size**: Use 1-10 million positions for best results
2. **Data Quality**: Include diverse positions from real games
3. **Learning Rate**: Start with 1.0, decrease if oscillating
4. **Iterations**: 500-2000 iterations typically sufficient
5. **Validation**: Test on separate validation set to avoid overfitting

## Performance Comparison

| Method | Speed | Strength | Tuning Required |
|--------|-------|----------|-----------------|
| Classic | Fast | Good | Yes (Texel) |
| NNUE | Very Fast* | Excellent | Yes (Training) |

*With incremental updates

### Typical Evaluation Times

- **Classic Eval**: 100-300 ns per position
- **NNUE Full**: 200-500 ns per position
- **NNUE Incremental**: 10-50 ns per move

## UCI Commands

### Evaluation Selection

```bash
# Use classic evaluation (default)
setoption name Use NNUE value false

# Use NNUE evaluation
setoption name Use NNUE value true
setoption name EvalFile value network.nnue
```

### Tuning Commands

```bash
# Enable tuning mode
setoption name Tuning Mode value true
setoption name Training Data value training.epd

# Run tuning
tune iterations 1000 lr 1.0

# Export parameters
exportparams eval_params.json
```

### Debug Commands

```bash
# Display current position and evaluation
d

# Enable debug output
setoption name Debug value true
```

## Testing

### Running Evaluation Tests

```bash
cd build
ctest -R test_eval
```

### Test Coverage

The test suite includes:
- Material value tests
- Pawn structure tests (doubled, isolated, passed, backward, chains)
- King safety tests (pawn shield, open files, tropism)
- Mobility tests
- Piece-specific tests (rook files, bishop pair, outposts)
- Endgame tests (king activity, opposition)
- Known position tests (Lucena, Philidor, etc.)
- Symmetry tests (horizontal, vertical, side-to-move)
- Consistency tests (determinism, make/unmake)

## Evaluation Tuning Workflow

### 1. Collect Training Data

```bash
# From game database (PGN)
# (PGN parser not yet implemented - use EPD format)

# Manual EPD format
echo "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 0.5" > training.epd
echo "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1 0.5" >> training.epd
# ... add more positions
```

### 2. Run Initial Evaluation

```bash
./chess_engine
setoption name Training Data value training.epd
```

### 3. Tune Parameters

```bash
setoption name Tuning Mode value true
tune iterations 1000 lr 1.0
```

### 4. Export and Validate

```bash
exportparams tuned_params.json
```

### 5. Test Strength

```bash
# Run benchmark with tuned parameters
bench 13

# Play test games against previous version
# (requires external testing framework like cutechess-cli)
```

## Advanced Topics

### Custom Evaluation Features

To add new evaluation features:

1. Add parameter to `EvalParams` struct in `classic_eval.h`
2. Initialize parameter in `ClassicEval::ClassicEval()`
3. Implement evaluation logic in appropriate function
4. Add parameter to tuner in `tuner.cpp`
5. Add tests in `test_eval.cpp`

### NNUE Training

For training custom NNUE networks:

1. Generate training data from games
2. Use external trainer (e.g., nnue-pytorch)
3. Convert trained network to supported format
4. Load and test in engine

### Evaluation Debugging

```bash
# Display detailed evaluation breakdown
setoption name Debug value true
d

# This will show:
# - Material count
# - Pawn structure scores
# - King safety scores
# - Mobility scores
# - Piece-specific bonuses
# - Total evaluation
```

## References

- [Texel Tuning Method](https://www.chessprogramming.org/Texel%27s_Tuning_Method)
- [NNUE Overview](https://www.chessprogramming.org/NNUE)
- [Stockfish Evaluation](https://github.com/official-stockfish/Stockfish/blob/master/src/evaluate.cpp)
- [Chess Programming Wiki](https://www.chessprogramming.org/Evaluation)

## Performance Tips

1. **Use NNUE for Strength**: NNUE typically adds 100-200 Elo over classic eval
2. **Tune Classic Eval**: Even if using NNUE, tune classic eval for fallback
3. **Profile Evaluation**: Use profiling tools to identify bottlenecks
4. **Cache Evaluations**: Search already caches in transposition table
5. **Incremental Updates**: Always enabled for NNUE, optional for classic

## Future Improvements

- [ ] Add contempt factor for draw avoidance
- [ ] Implement evaluation caching beyond TT
- [ ] Add more sophisticated king safety (attack units)
- [ ] Implement space evaluation
- [ ] Add piece coordination bonuses
- [ ] Support for larger NNUE networks (512, 1024 hidden)
- [ ] Multi-threaded Texel tuning
- [ ] PGN parser for training data generation
