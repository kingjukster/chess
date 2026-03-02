# Chess Engine Evaluation System

This directory contains the evaluation system for the chess engine, including classic hand-crafted evaluation and Texel tuning framework.

## Files

### Core Evaluation

- **`evaluator.h`** - Base evaluator interface
- **`classic_eval.h`** - Classic evaluator interface with tunable parameters
- **`classic_eval.cpp`** - Complete implementation with 19 evaluation features

### Tuning Framework

- **`tuner.h`** - Texel tuning framework interface
- **`tuner.cpp`** - Gradient descent optimization implementation

### Documentation

- **`EVALUATION_GUIDE.md`** - Complete system architecture and usage
- **`TUNING_GUIDE.md`** - Step-by-step tuning guide
- **`QUICK_START.md`** - Quick reference for common tasks

### Example Data

- **`example_training.epd`** - Sample training data for tuning

## Quick Start

### 1. Use Classic Evaluation

```bash
./chess_engine
setoption name Use NNUE value false
position startpos
d
```

### 2. Tune Evaluation Parameters

```bash
./chess_engine
setoption name Tuning Mode value true
setoption name Training Data value example_training.epd
tune iterations 100 lr 1.0
exportparams tuned.json
```

### 3. Run Tests

```bash
cd ../build
ctest -R test_eval
```

## Evaluation Features

### Implemented (19 features)

1. **Material Values** - Base piece values
2. **Piece-Square Tables** - Position-dependent bonuses
3. **Doubled Pawns** - Penalty for pawns on same file
4. **Isolated Pawns** - Penalty for unsupported pawns
5. **Passed Pawns** - Bonus for advanced passed pawns
6. **Backward Pawns** - Penalty for vulnerable pawns
7. **Pawn Chains** - Bonus for connected pawns
8. **Pawn Shield** - King protection bonus
9. **Open Files Near King** - King exposure penalty
10. **King Tropism** - Enemy piece proximity penalty
11. **Knight Mobility** - Legal move count bonus
12. **Bishop Mobility** - Legal move count bonus
13. **Rook Mobility** - Legal move count bonus
14. **Queen Mobility** - Legal move count bonus
15. **Rook Open File** - Open file bonus
16. **Rook Semi-Open File** - Semi-open file bonus
17. **Bishop Pair** - Two bishops bonus
18. **Knight Outpost** - Protected square bonus
19. **Bad Bishop** - Blocked bishop penalty
20. **King Activity** - Endgame centralization
21. **Opposition** - Pawn endgame advantage
22. **Phase Detection** - Smooth game phase transition

### Tunable Parameters (30+)

All evaluation parameters can be optimized using Texel tuning:
- 5 material values
- 12 pawn structure parameters
- 3 king safety parameters
- 4 mobility weights
- 5 piece-specific bonuses
- 2 endgame parameters

## Architecture

### Classic Evaluation Pipeline

```
Position
    ↓
Material + PST (phase-dependent)
    ↓
Pawn Structure Analysis
    ↓
King Safety Evaluation
    ↓
Mobility Calculation
    ↓
Piece-Specific Bonuses
    ↓
Endgame Features
    ↓
Final Score (centipawns)
```

### Texel Tuning Pipeline

```
Training Data (EPD)
    ↓
Load Positions
    ↓
Calculate Error (MSE)
    ↓
Compute Gradients
    ↓
Update Parameters
    ↓
Repeat until Convergence
    ↓
Export Tuned Parameters
```

## Performance

### Evaluation Speed

- **Classic (Basic)**: ~100 ns/position
- **Classic (Full)**: ~300 ns/position
- **Tuning**: ~10-30 minutes per 1000 iterations (1M positions)

### Strength Improvement

Estimated Elo gains from complete evaluation:
- Pawn Structure: +50-80 Elo
- King Safety: +30-50 Elo
- Mobility: +40-60 Elo
- Piece-Specific: +30-40 Elo
- Endgame: +20-30 Elo
- **Total**: +150-250 Elo

## Testing

### Test Suite

34 comprehensive tests covering:
- Material values and hierarchy
- Pawn structure (all 5 types)
- King safety (3 aspects)
- Mobility
- Piece-specific features (4 types)
- Endgame features (2 types)
- Known positions (6 positions)
- Symmetry (3 types)
- Consistency (3 types)

### Running Tests

```bash
cd ../build
ctest -R test_eval -V
```

## Documentation

### For Users

- **`QUICK_START.md`** - Get started quickly
- **`EVALUATION_GUIDE.md`** - Understand the system
- **`TUNING_GUIDE.md`** - Optimize parameters

### For Developers

- **`classic_eval.h`** - API documentation
- **`tuner.h`** - Tuning framework API
- **`../tests/test_eval.cpp`** - Test examples

## Examples

### Example 1: Evaluate Position

```cpp
#include "eval/classic_eval.h"

ClassicEval evaluator;
Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
evaluator.initialize(pos);
int score = evaluator.evaluate(pos);
std::cout << "Evaluation: " << score << " cp" << std::endl;
```

### Example 2: Tune Parameters

```cpp
#include "eval/tuner.h"

TexelTuner tuner;
ClassicEval evaluator;

tuner.set_evaluator(&evaluator);
tuner.load_training_data("training.epd");
tuner.tune(1000, 1.0);
tuner.save_parameters("tuned.txt");
```

### Example 3: Custom Parameters

```cpp
ClassicEval evaluator;
ClassicEval::EvalParams params = evaluator.get_params();

// Modify parameters
params.bishop_pair_bonus = 35;
params.rook_open_file_bonus = 30;

evaluator.set_params(params);
```

## Contributing

When adding new evaluation features:

1. Add parameter to `EvalParams` struct
2. Initialize in constructor
3. Implement evaluation logic
4. Add to tuner parameter list
5. Add tests
6. Update documentation

## References

- [Chess Programming Wiki - Evaluation](https://www.chessprogramming.org/Evaluation)
- [Stockfish Evaluation](https://github.com/official-stockfish/Stockfish)
- [Texel Tuning](https://www.chessprogramming.org/Texel%27s_Tuning_Method)

## License

Part of the chess engine project.
