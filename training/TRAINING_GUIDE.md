# Complete Guide to Training Your Own NNUE Network

## Overview

This guide will walk you through the complete process of training a NNUE (Efficiently Updatable Neural Network) evaluation function for your chess engine.

## Prerequisites

1. **Python 3.8+** with pip
2. **PyTorch** (for training)
3. **Chess libraries** (for position handling)

Install dependencies:
```bash
cd training
pip install -r requirements.txt
```

## Step-by-Step Process

### Step 1: Get Training Data

You have three options:

#### Option A: Use Real Game Data (Best Quality)

1. Download PGN files from:
   - **Lichess**: https://database.lichess.org/ (free, millions of games)
   - **Chess.com**: Requires API access
   - **FICS**: Free Internet Chess Server games

2. Example download:
   ```bash
   # Download a month of Lichess games (example)
   wget https://database.lichess.org/standard/lichess_db_standard_rated_2023-01.pgn.zst
   # Decompress
   unzstd lichess_db_standard_rated_2023-01.pgn.zst
   ```

#### Option B: Generate Synthetic Data (Quick Start)

For testing, you can generate random positions:

```bash
python training/generate_synthetic_data.py --output training_data.bin --num-positions 50000
```

This creates positions with simple material-based evaluations.

#### Option C: Use Stockfish to Generate Evaluations

If you have Stockfish installed, you can use it to evaluate positions:

```python
# Example: evaluate positions with Stockfish
import chess
import chess.engine

engine = chess.engine.SimpleEngine.popen_uci("stockfish")
board = chess.Board()
info = engine.analyse(board, chess.engine.Limit(depth=15))
eval_score = info["score"].white().score(mate_score=30000)
```

### Step 2: Prepare Training Data

Convert your PGN file or positions into the training format:

```bash
python training/prepare_data.py \
    --input games.pgn \
    --output training_data.bin \
    --max-positions 1000000
```

**What this does:**
- Reads games from PGN file
- Extracts positions (samples every 5th move after move 5)
- Converts positions to HalfKP features
- Evaluates positions (using Stockfish if available, or simple heuristic)
- Saves in binary format for efficient training

**Output format:**
- Binary file with positions, features, and evaluations
- Each position includes:
  - Evaluation score (centipawns)
  - Side to move
  - HalfKP feature indices

### Step 3: Train the Network

Train your NNUE network:

```bash
python training/train_nnue.py \
    --data training_data.bin \
    --epochs 20 \
    --batch-size 1024 \
    --learning-rate 0.001 \
    --output nnue_model.pth
```

**Parameters:**
- `--epochs`: Number of training epochs (10-50 typical)
- `--batch-size`: Batch size (512-2048, depends on GPU memory)
- `--learning-rate`: Learning rate (0.0001-0.01)
- `--device`: `cuda` (GPU) or `cpu`

**Training process:**
1. Loads training data
2. Splits into train/validation (90/10)
3. Trains network with Adam optimizer
4. Uses MSE loss (mean squared error)
5. Saves best model based on validation loss

**Expected output:**
```
Epoch 1/20
Training... 100%|████████| 1000/1000 [02:30<00:00]
Train Loss: 2.3456, Val Loss: 2.1234
Saved best model to nnue_model.pth

Epoch 2/20
...
```

### Step 4: Export Weights

Convert the trained PyTorch model to your engine's format:

```bash
python training/export_weights.py \
    --model nnue_model.pth \
    --output ../nnue_weights.bin \
    --scale 64
```

**What this does:**
- Loads PyTorch model
- Quantizes float32 weights to int16_t
- Saves in engine's binary format
- Applies quantization scale factor

**Quantization:**
- Weights are scaled by `--scale` factor (default 64)
- Clamped to int16 range (-32768 to 32767)
- This matches the engine's int16_t weight format

### Step 5: Use in Engine

Load the weights in your engine:

**Option A: Via UCI (add to uci.cpp):**
```cpp
if (name == "EvalFile") {
    std::string filename;
    iss >> filename;
    if (use_nnue && evaluator) {
        static_cast<NnueEval*>(evaluator)->load_network(filename);
    }
}
```

**Option B: In initialization:**
```cpp
NnueEval* nnue = new NnueEval();
nnue->load_network("nnue_weights.bin");
```

## Quick Start (All-in-One)

For a quick test run:

```bash
cd training

# 1. Generate synthetic data (or use real PGN)
python generate_synthetic_data.py --output data.bin --num-positions 50000

# 2. Train
python train_nnue.py --data data.bin --epochs 10 --output model.pth

# 3. Export
python export_weights.py --model model.pth --output ../nnue_weights.bin

# 4. Test in engine
cd ..
./chess_engine
# In UCI: setoption name Use NNUE value true
```

## Improving Your Network

### 1. Better Training Data

- Use more positions (1M+ recommended)
- Use stronger engine evaluations (Stockfish depth 15+)
- Filter positions (remove draws, focus on interesting positions)
- Balance positions (equal number of winning/losing positions)

### 2. Network Architecture

Modify `train_nnue.py` to experiment:
- Hidden layer size: 128, 256, 512, 1024
- Multiple hidden layers
- Different activation functions

### 3. Training Techniques

- **Learning rate scheduling**: Already included (reduces LR every 5 epochs)
- **Data augmentation**: Flip board, rotate features
- **Regularization**: Add dropout or L2 regularization
- **Early stopping**: Stop if validation loss doesn't improve

### 4. Evaluation Quality

- Use deeper Stockfish searches (depth 20+)
- Use multiple engines and average evaluations
- Use game outcomes (win/loss/draw) as labels
- Use self-play data from your engine

## Troubleshooting

### "Out of memory" during training
- Reduce `--batch-size` (try 256 or 512)
- Use fewer positions
- Use CPU instead of GPU

### Poor evaluation quality
- Train for more epochs
- Use better training data (Stockfish evaluations)
- Increase network size
- Check feature extraction matches engine

### Weights don't load in engine
- Check file format matches (use `export_weights.py`)
- Verify quantization scale matches engine expectations
- Check file path is correct

### Training loss not decreasing
- Learning rate too high/low (try 0.0001 or 0.01)
- Check data quality (evaluations make sense?)
- Verify feature extraction is correct
- Try different optimizer (SGD instead of Adam)

## Advanced: Self-Play Training

For best results, use self-play:

1. Generate positions by playing your engine against itself
2. Evaluate with strong engine (Stockfish)
3. Train on these positions
4. Repeat (the network improves, generates better positions)

## Resources

- **Stockfish NNUE**: Reference implementation
  - https://github.com/official-stockfish/Stockfish
  - See `src/nnue/` for optimized code

- **NNUE Paper**: "Efficiently Updatable Neural-Network-based Evaluation Functions"
  - Explains HalfKP features and architecture

- **Training Data**:
  - Lichess database: https://database.lichess.org/
  - Chess.com API: https://www.chess.com/news/view/published-data-api

## Next Steps After Training

1. **Test the network**: Play games, compare with ClassicEval
2. **Optimize**: Add SIMD, improve accumulator updates
3. **Tune**: Adjust search parameters for NNUE
4. **Iterate**: Train better networks with more data

## Example Training Session

```bash
# Download games
wget https://database.lichess.org/standard/lichess_db_standard_rated_2023-01.pgn.zst
unzstd lichess_db_standard_rated_2023-01.pgn.zst

# Prepare data (this takes a while)
python prepare_data.py --input lichess_db_standard_rated_2023-01.pgn \
                       --output training_data.bin \
                       --max-positions 500000

# Train (with GPU, takes ~1 hour for 500k positions)
python train_nnue.py --data training_data.bin \
                     --epochs 20 \
                     --batch-size 2048 \
                     --device cuda \
                     --output nnue_model.pth

# Export
python export_weights.py --model nnue_model.pth \
                         --output ../nnue_weights.bin

# Test!
cd ..
./chess_engine
# setoption name Use NNUE value true
# position startpos
# go depth 10
```

Good luck training! 🚀

