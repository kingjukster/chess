# How to Use Trained NNUE Weights

## Quick Guide

After training your network and exporting weights, here's how to use them in the engine.

## Step 1: Train and Export

```bash
cd training

# Train your network (see TRAINING_GUIDE.md)
python train_nnue.py --data training_data.bin --epochs 10 --output model.pth

# Export to engine format
python export_weights.py --model model.pth --output ../nnue_weights.bin
```

This creates `nnue_weights.bin` in the main directory.

## Step 2: Build the Engine

```bash
cd ..
mkdir build
cd build
cmake ..
make  # or cmake --build . on Windows
```

## Step 3: Use via UCI

Run the engine and use UCI commands:

```bash
./chess_engine  # or chess_engine.exe on Windows
```

Then in the UCI interface:

```
uci
setoption name EvalFile value nnue_weights.bin
setoption name Use NNUE value true
isready
position startpos
go depth 10
```

## UCI Commands Explained

1. **`setoption name EvalFile value nnue_weights.bin`**
   - Sets the path to your NNUE network file
   - Can be relative or absolute path
   - Must be set BEFORE enabling NNUE

2. **`setoption name Use NNUE value true`**
   - Switches from ClassicEval to NnueEval
   - Automatically loads the network file if EvalFile was set
   - Use `value false` to switch back to ClassicEval

3. **`isready`**
   - Checks if engine is ready
   - Network loading happens here if not already loaded

## Example Session

```
> uci
id name ChessEngine
id author NNUE Engine
option name Use NNUE type check default false
option name EvalFile type string default <empty>
uciok

> setoption name EvalFile value nnue_weights.bin

> setoption name Use NNUE value true

> isready
readyok

> position startpos

> go depth 8
info depth 8 score cp 25 nodes 12345
bestmove e2e4
```

## Troubleshooting

### "Failed to open network file"
- Check the file path is correct
- Use absolute path if relative doesn't work: `C:\Users\horne\projects\chess_engine\chess\nnue_weights.bin`
- Make sure the file exists

### "Failed to load NNUE network, using default"
- Network file format might be wrong
- Make sure you used `export_weights.py` to create the file
- Check file isn't corrupted

### Engine crashes when loading
- Make sure you built with the latest code
- Check that `nnue_loader.cpp` is included in CMakeLists.txt
- Verify network file size matches expected format

## File Locations

```
chess/
├── nnue_weights.bin          # Your trained network (put here)
├── chess_engine.exe           # Compiled engine
└── training/
    ├── model.pth              # PyTorch model (training)
    └── export_weights.py      # Export script
```

## Alternative: Hardcode Path

If you want to always load a specific network, you can modify `uci.cpp`:

```cpp
// In UCI::UCI() constructor, after creating NnueEval:
if (use_nnue) {
    NnueEval* nnue = static_cast<NnueEval*>(evaluator);
    nnue->load_network("nnue_weights.bin");  // Hardcoded path
}
```

But using UCI options is more flexible!

