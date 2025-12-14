# Training Your Own NNUE Network

This guide will walk you through training a NNUE network for the chess engine.

## Overview

Training a NNUE network requires:
1. **Training data**: Positions with evaluations (from strong engines or games)
2. **Feature extraction**: Convert positions to HalfKP features
3. **Model training**: Train a neural network
4. **Export**: Convert trained weights to engine format

## Prerequisites

```bash
pip install torch numpy chess python-chess
```

## Step 1: Collect Training Data

### Option A: Use Game Databases

Download games from:
- Lichess database: https://database.lichess.org/
- Chess.com games
- FICS games

### Option B: Generate with Self-Play

Use a strong engine (Stockfish) to generate positions and evaluations.

## Step 2: Prepare Training Data

Run `prepare_data.py` to convert games/positions into training format.

## Step 3: Train the Network

Run `train_nnue.py` to train the network.

## Step 4: Export Weights

Run `export_weights.py` to convert trained model to engine format.

## Quick Start

```bash
# 1. Prepare data
python training/prepare_data.py --input games.pgn --output training_data.bin

# 2. Train
python training/train_nnue.py --data training_data.bin --epochs 10

# 3. Export
python training/export_weights.py --model model.pth --output nnue_weights.bin
```

