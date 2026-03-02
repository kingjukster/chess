# Evaluation System Quick Start

## Using Classic Evaluation

### Basic Usage

```bash
./chess_engine
uci
setoption name Use NNUE value false
position startpos
d
```

The `d` command displays the current position and evaluation score.

### View Evaluation Components

Enable debug mode to see detailed breakdown:

```bash
setoption name Debug value true
d
```

## Using NNUE Evaluation

### With Stockfish Network

```bash
./chess_engine
uci
setoption name Use NNUE value true
setoption name EvalFile value nn-0000000000a0.nnue
position startpos
go depth 10
```

### Converting Networks

```bash
./nnue_converter stockfish.nnue native.bin
```

Then use the native format:

```bash
setoption name EvalFile value native.bin
```

## Tuning Evaluation Parameters

### Quick Tuning (Small Dataset)

```bash
./chess_engine
uci
setoption name Tuning Mode value true
setoption name Training Data value example_training.epd
tune iterations 100 lr 1.0
exportparams quick_tuned.json
```

### Full Tuning (Large Dataset)

```bash
# Prepare training data (1M+ positions)
# Format: <FEN> <result>
# Result: 1.0 (white win), 0.5 (draw), 0.0 (black win)

./chess_engine
uci
setoption name Tuning Mode value true
setoption name Training Data value training_1M.epd
tune iterations 1000 lr 1.0
exportparams tuned_params.json
```

## Testing Evaluation

### Run All Evaluation Tests

```bash
cd build
ctest -R test_eval -V
```

### Run Specific Test

```bash
cd build
./chess_tests --gtest_filter=EvalTest.PassedPawns
```

### Test Categories

- **Material Tests**: Basic piece values
- **Pawn Structure Tests**: Doubled, isolated, passed pawns
- **King Safety Tests**: Pawn shield, open files
- **Mobility Tests**: Piece mobility scoring
- **Endgame Tests**: King activity, opposition
- **Known Positions**: Lucena, Philidor, etc.
- **Symmetry Tests**: Position symmetry validation
- **Consistency Tests**: Determinism, make/unmake

## Evaluation Parameters

### Default Values

```
Material:
  Pawn:   100 cp
  Knight: 320 cp
  Bishop: 330 cp
  Rook:   500 cp
  Queen:  900 cp

Pawn Structure:
  Doubled:  -10 cp
  Isolated: -15 cp
  Backward:  -8 cp
  Chain:     +5 cp
  Passed:   10-200 cp (by rank)

King Safety:
  Pawn Shield:    +10 cp per pawn
  Open File:      -20 cp per file
  King Tropism:    -5 cp per unit

Mobility:
  Knight: +4 cp per move
  Bishop: +3 cp per move
  Rook:   +2 cp per move
  Queen:  +1 cp per move

Piece-Specific:
  Rook Open File:      +25 cp
  Rook Semi-Open:      +12 cp
  Bishop Pair:         +30 cp
  Knight Outpost:      +15 cp
  Bad Bishop:          -20 cp

Endgame:
  King Activity:  +10 cp
  Opposition:     +20 cp
```

## Common Tasks

### Compare Evaluations

```bash
# Position 1
position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
d
# Note the evaluation

# Position 2
position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1
d
# Compare evaluations (should be opposite sign)
```

### Benchmark Evaluation Speed

```bash
bench 13
```

Shows nodes per second and evaluation performance.

### Export Current Parameters

```bash
exportparams current_params.json
```

Exports current evaluation parameters in JSON format.

## Troubleshooting

### Evaluation Seems Wrong

1. Check if NNUE is enabled: `setoption name Use NNUE value false`
2. Verify position is correct: `d` command
3. Enable debug mode: `setoption name Debug value true`
4. Run evaluation tests: `ctest -R test_eval`

### Tuning Not Working

1. Verify training data format (FEN + result)
2. Check file path is correct
3. Ensure tuning mode is enabled
4. Try smaller dataset first (100-1000 positions)

### NNUE Not Loading

1. Check file path is correct
2. Verify network format (use auto-detection)
3. Try converting with `nnue_converter`
4. Check file size (should be 20-40 MB)

### Tests Failing

1. Ensure Attacks::init() is called
2. Check FEN parsing is correct
3. Verify evaluation is symmetric
4. Run with `--gtest_filter=EvalTest.*` for details

## Performance Tips

### For Speed

- Use NNUE with incremental updates
- Disable debug mode in production
- Use optimized build (`-O3` or `/O2`)
- Enable SIMD instructions (AVX2)

### For Strength

- Use NNUE evaluation
- Tune classic evaluation as fallback
- Use larger NNUE networks (256+ hidden)
- Train on diverse position set

### For Tuning

- Use 1M+ positions for best results
- Balance wins/draws/losses in training data
- Run multiple tuning sessions with different learning rates
- Validate on separate test set

## Next Steps

1. **Generate Training Data**: Extract positions from game databases
2. **Run Tuning**: Optimize evaluation parameters
3. **Test Strength**: Play games against previous version
4. **Iterate**: Refine based on results

## Documentation

- **Evaluation Guide**: `eval/EVALUATION_GUIDE.md`
- **Tuning Guide**: `eval/TUNING_GUIDE.md`
- **NNUE Format**: `nnue/NNUE_FORMAT.md`
- **Test Suite**: `tests/test_eval.cpp`

## Support Files

- **Example Training Data**: `eval/example_training.epd`
- **Parameter Export**: `exportparams` command
- **Network Converter**: `nnue_converter` utility
