#!/bin/bash
# Quick start script for training NNUE network

set -e

echo "=== NNUE Training Quick Start ==="
echo ""

# Check dependencies
echo "Checking dependencies..."
python3 -c "import torch; import chess; import numpy" 2>/dev/null || {
    echo "Installing dependencies..."
    pip install -r requirements.txt
}

# Step 1: Prepare data
if [ ! -f "training_data.bin" ]; then
    echo ""
    echo "Step 1: Preparing training data..."
    echo "You need a PGN file. Download one from:"
    echo "  https://database.lichess.org/"
    echo ""
    read -p "Enter path to PGN file (or press Enter to skip): " pgn_file
    
    if [ -n "$pgn_file" ] && [ -f "$pgn_file" ]; then
        python3 prepare_data.py --input "$pgn_file" --output training_data.bin --max-positions 100000
    else
        echo "Skipping data preparation. Create training_data.bin manually."
    fi
else
    echo "Found training_data.bin, skipping preparation."
fi

# Step 2: Train
if [ -f "training_data.bin" ]; then
    echo ""
    echo "Step 2: Training network..."
    python3 train_nnue.py --data training_data.bin --epochs 10 --output nnue_model.pth
else
    echo "No training_data.bin found. Skipping training."
fi

# Step 3: Export
if [ -f "nnue_model.pth" ]; then
    echo ""
    echo "Step 3: Exporting weights..."
    python3 export_weights.py --model nnue_model.pth --output ../nnue_weights.bin
    echo ""
    echo "Done! Weights saved to nnue_weights.bin"
    echo "Copy this file to your engine directory and load it via UCI."
else
    echo "No model found. Skipping export."
fi

