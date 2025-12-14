# Quick Start Guide

## Build Complete! ✅

Your chess engine has been successfully built at:
```
build\Release\chess_engine.exe
```

## Testing the Engine

### 1. Basic UCI Test

Run the engine:
```powershell
cd build\Release
.\chess_engine.exe
```

Then type these UCI commands:
```
uci
isready
position startpos
go depth 5
quit
```

### 2. Test with Classic Evaluation

```
uci
setoption name Use NNUE value false
isready
position startpos
go depth 6
```

### 3. Test with NNUE Evaluation

First, make sure your trained network is in the main directory:
```
chess\
├── nnue_weights.bin    ← Your trained network
└── build\
    └── Release\
        └── chess_engine.exe
```

Then:
```
uci
setoption name EvalFile value ..\..\nnue_weights.bin
setoption name Use NNUE value true
isready
position startpos
go depth 6
```

### 4. Perft Testing

Verify move generation correctness:
```
uci
perft 1
perft 2
perft 3
```

Expected results:
- Depth 1: 20 moves
- Depth 2: 400 moves  
- Depth 3: 8902 moves

## Using with Chess GUIs

### Arena Chess GUI
1. Download Arena from http://www.playwitharena.com/
2. Engine → Install New Engine
3. Browse to `build\Release\chess_engine.exe`
4. The engine should appear in the engine list

### Cute Chess
1. Download Cute Chess from https://cutechess.com/
2. Tools → Engine Management
3. Add new engine, point to `chess_engine.exe`
4. Start a game!

### Lichess/Chess.com
The engine supports UCI protocol, so it should work with any UCI-compatible interface.

## Troubleshooting

### "Failed to open network file"
- Make sure `nnue_weights.bin` is in the correct location
- Use absolute path if relative doesn't work:
  ```
  setoption name EvalFile value C:\Users\horne\projects\chess_engine\chess\nnue_weights.bin
  ```

### Engine doesn't respond
- Make sure you're typing UCI commands (not PowerShell commands)
- Type `uci` first to initialize
- Check that the executable is running

### Engine plays poorly
- This is expected with default/random NNUE weights
- Train a better network with more data (see `training/TRAINING_GUIDE.md`)
- Or use ClassicEval for now: `setoption name Use NNUE value false`

## Next Steps

1. **Test the engine** - Play some games, verify it works
2. **Improve the network** - Train with better data (see training guide)
3. **Optimize** - Add SIMD, improve search, tune parameters
4. **Have fun!** - Play against your engine, analyze positions

Enjoy your chess engine! 🎉

