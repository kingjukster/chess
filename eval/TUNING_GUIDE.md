# Texel Tuning Guide

This guide explains how to use the Texel tuning framework to optimize evaluation parameters.

## What is Texel Tuning?

Texel tuning is an automated method for optimizing chess evaluation parameters using a large dataset of positions with known game outcomes. It was developed by Peter Österlund for the Texel chess engine.

### How It Works

1. **Training Data**: Collect positions from real games with known results (win/draw/loss)
2. **Error Function**: Calculate how well the evaluation predicts game outcomes
3. **Optimization**: Use gradient descent to minimize prediction error
4. **Validation**: Test tuned parameters against original parameters

## Preparing Training Data

### Format

Training data should be in EPD format with results:

```
<FEN> <result>
```

Where:
- `<FEN>`: Standard FEN notation for the position
- `<result>`: Game outcome (1.0 = white win, 0.5 = draw, 0.0 = black win)

### Example Training Data

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 0.5
r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3 0.5
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 1.0
8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1 0.0
```

### Generating Training Data

#### Option 1: From PGN Files (Manual)

1. Extract positions from games at regular intervals (e.g., every 8 moves)
2. Skip positions in the opening (first 10 moves)
3. Skip positions near the end (last 5 moves)
4. Assign result based on game outcome
5. Filter out positions with forced sequences (optional)

#### Option 2: From Engine Games

1. Play games between engines at fixed depth
2. Extract positions from each game
3. Use game result as position result
4. Aim for 1-10 million positions

### Data Quality Tips

1. **Diversity**: Include positions from various openings and game phases
2. **Balance**: Roughly equal distribution of wins/draws/losses
3. **Quiet Positions**: Avoid tactical positions with forced wins
4. **Game Phase**: Include opening, middlegame, and endgame positions
5. **Size**: More data = better results (aim for 1M+ positions)

## Running Texel Tuning

### Step 1: Prepare the Engine

```bash
./chess_engine
```

### Step 2: Configure Tuning

```
setoption name Tuning Mode value true
setoption name Training Data value training.epd
```

### Step 3: Run Tuning

```
tune iterations 1000 lr 1.0
```

Parameters:
- `iterations`: Number of optimization iterations (500-2000 recommended)
- `lr`: Learning rate (0.5-2.0, start with 1.0)

### Step 4: Export Results

```
exportparams tuned_params.json
```

## Tuning Parameters

### Learning Rate

- **Too High**: Oscillation, no convergence
- **Too Low**: Slow convergence
- **Adaptive**: The tuner automatically adjusts learning rate

Recommended starting values:
- Material parameters: lr = 1.0
- Positional parameters: lr = 0.5
- Fine-tuning: lr = 0.1

### Iterations

- **Quick tuning**: 100-200 iterations
- **Standard tuning**: 500-1000 iterations
- **Deep tuning**: 1000-2000 iterations

The tuner will stop early if error stops improving.

## Understanding Results

### Error Metrics

The tuner reports Mean Squared Error (MSE):

```
error = Σ(sigmoid(eval) - result)² / N
```

Where `sigmoid(x) = 1 / (1 + e^(-x/400))` converts centipawn scores to win probabilities.

**Good error values:**
- < 0.10: Excellent fit
- 0.10-0.15: Good fit
- 0.15-0.20: Acceptable fit
- \> 0.20: Poor fit (need more data or iterations)

### Parameter Validation

After tuning, verify parameters are reasonable:

1. **Material Values**:
   - Pawn: 80-120
   - Knight: 280-360
   - Bishop: 280-370
   - Rook: 450-550
   - Queen: 850-1000

2. **Positional Bonuses**: Should be < 50 centipawns
3. **Penalties**: Should be positive and < 30 centipawns
4. **Passed Pawns**: Should increase with rank

## Example Tuning Session

```
$ ./chess_engine
id name ChessEngine
id author NNUE Engine
...
uciok

> setoption name Tuning Mode value true
info string Tuning mode enabled

> setoption name Training Data value training_1M.epd
info string Training data file set to: training_1M.epd

> tune iterations 1000 lr 1.0
Loaded 1000000 training positions
Starting Texel tuning with 1000000 positions
Iterations: 1000, Learning rate: 1.0
Initial error: 0.1523

Iteration 0: error = 0.1523
Iteration 10: error = 0.1489 (improved by 0.0034)
Iteration 20: error = 0.1461 (improved by 0.0028)
...
Iteration 990: error = 0.1102 (improved by 0.0002)
Iteration 999: error = 0.1101 (improved by 0.0001)

Tuning complete. Final error: 0.1101

Tuned parameters:
  material_pawn = 98
  material_knight = 325
  material_bishop = 335
  material_rook = 497
  material_queen = 912
  ...

> exportparams tuned_params.json
info string Parameters exported to: tuned_params.json

> quit
```

## Advanced Tuning Techniques

### 1. Parameter Constraints

Some parameters should maintain relationships:
- Bishop ≈ Knight (within 20 centipawns)
- Rook ≈ Knight + 2 Pawns
- Queen ≈ 2 Rooks

The tuner doesn't enforce constraints, so validate manually.

### 2. Multi-Stage Tuning

For best results, tune in stages:

**Stage 1: Material Values**
- Tune only material values first
- Use diverse positions
- 500 iterations

**Stage 2: Positional Features**
- Fix material values
- Tune pawn structure, king safety, mobility
- 1000 iterations

**Stage 3: Fine-Tuning**
- Tune all parameters together
- Lower learning rate (0.1-0.5)
- 500 iterations

### 3. Validation

After tuning:

1. **Self-Play**: Play games against untuned version
2. **Test Suite**: Run on standard test positions (WAC, STS, etc.)
3. **Perft**: Verify no bugs introduced
4. **Benchmark**: Check performance hasn't degraded

## Troubleshooting

### High Initial Error (> 0.20)

- Check training data quality
- Verify FEN parsing is correct
- Ensure results are properly formatted
- Try smaller dataset first

### No Improvement

- Increase learning rate
- Check for bugs in evaluation code
- Verify gradient calculation
- Try local search instead of gradient descent

### Oscillating Error

- Decrease learning rate
- Use adaptive learning rate (automatic)
- Add momentum term (not yet implemented)

### Unreasonable Parameters

- Add parameter constraints
- Use smaller learning rate
- Validate training data
- Check for evaluation bugs

## Performance Tips

### Memory Usage

- Large datasets (10M positions) require ~1-2 GB RAM
- Consider batch processing for very large datasets

### Speed

- Tuning 1M positions takes ~10-30 minutes per iteration
- Use faster evaluation (disable expensive features during tuning)
- Consider parallel evaluation (not yet implemented)

### Convergence

- Monitor error every 10 iterations
- Stop if error hasn't improved in 50 iterations
- Save intermediate results

## Integration with Engine

After tuning, integrate parameters:

1. **Update Defaults**: Modify `ClassicEval::ClassicEval()` constructor
2. **Recompile**: Build engine with new defaults
3. **Test**: Verify improvement in playing strength
4. **Iterate**: Tune again with more data if needed

## Example Training Data Generation

### From Lichess Database

```python
import chess.pgn
import random

def extract_positions(pgn_file, output_file, positions_per_game=5):
    with open(pgn_file) as pgn, open(output_file, 'w') as out:
        while True:
            game = chess.pgn.read_game(pgn)
            if game is None:
                break
            
            result = game.headers["Result"]
            if result == "1-0":
                result_value = 1.0
            elif result == "0-1":
                result_value = 0.0
            elif result == "1/2-1/2":
                result_value = 0.5
            else:
                continue
            
            board = game.board()
            moves = list(game.mainline_moves())
            
            # Skip very short games
            if len(moves) < 20:
                continue
            
            # Extract positions at regular intervals
            positions = []
            for i in range(10, len(moves) - 5, len(moves) // positions_per_game):
                board.push(moves[i])
                positions.append(board.fen())
            
            # Write positions
            for fen in positions:
                out.write(f"{fen} {result_value}\n")

extract_positions("lichess_db.pgn", "training.epd")
```

## References

- [Texel's Tuning Method](https://www.chessprogramming.org/Texel%27s_Tuning_Method)
- [Evaluation Tuning](https://www.chessprogramming.org/Automated_Tuning)
- [Stockfish Tuning](https://github.com/official-stockfish/Stockfish/wiki/Regression-Tests)
