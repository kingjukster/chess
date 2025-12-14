# How to Run Your Chess Engine

## Quick Start

### Step 1: Navigate to the executable

```powershell
cd build\Release
```

### Step 2: Run the engine

```powershell
.\chess_engine.exe
```

### Step 3: Type UCI commands

The engine will start and wait for UCI commands. Type these:

```
uci
isready
position startpos
go depth 3
quit
```

## Complete Example Session

Here's a full example of running the engine:

```powershell
PS C:\Users\horne\projects\chess_engine\chess\build\Release> .\chess_engine.exe
id name ChessEngine
id author NNUE Engine
option name Use NNUE type check default false
option name EvalFile type string default <empty>
uciok

> uci
id name ChessEngine
id author NNUE Engine
option name Use NNUE type check default false
option name EvalFile type string default <empty>
uciok

> isready
readyok

> position startpos

> go depth 3
info depth 1 nodes 20 score cp 25 pv e2e4
info depth 2 nodes 400 score cp 15 pv e2e4
info depth 3 nodes 8902 score cp 20 pv e2e4
bestmove e2e4

> quit
```

## Common UCI Commands

### Basic Commands
- `uci` - Initialize UCI protocol
- `isready` - Check if engine is ready
- `quit` - Exit the engine

### Position Commands
- `position startpos` - Set to starting position
- `position startpos moves e2e4 e7e5` - Set position and play moves
- `position fen <fen_string>` - Set position from FEN

### Search Commands
- `go depth 5` - Search to depth 5
- `go wtime 5000 btime 5000` - Search with time controls (5 seconds)
- `go infinite` - Search until stopped
- `stop` - Stop current search

### Options
- `setoption name Use NNUE value true` - Enable NNUE evaluation
- `setoption name Use NNUE value false` - Use classic evaluation
- `setoption name EvalFile value nnue_weights.bin` - Load NNUE network

### Testing
- `perft 3` - Run perft test (move generation verification)

## Using with NNUE

1. **Load your trained network:**
   ```
   setoption name EvalFile value ..\..\nnue_weights.bin
   setoption name Use NNUE value true
   ```

2. **Search:**
   ```
   position startpos
   go depth 5
   ```

## Using with Chess GUIs

### Arena Chess GUI

1. Download Arena: http://www.playwitharena.com/
2. Install and open Arena
3. **Engine → Install New Engine**
4. Browse to: `C:\Users\horne\projects\chess_engine\chess\build\Release\chess_engine.exe`
5. The engine will appear in your engine list
6. Right-click engine → **Configure** to set options
7. Start a game!

### Cute Chess

1. Download: https://cutechess.com/
2. **Tools → Engine Management**
3. Click **+** to add engine
4. Name: "ChessEngine"
5. Command: Browse to `chess_engine.exe`
6. Protocol: UCI
7. Click **OK**
8. Start a game!

### Command Line Testing

You can also pipe commands to the engine:

```powershell
echo "uci`nisready`nposition startpos`ngo depth 3`nquit" | .\chess_engine.exe
```

## Troubleshooting

### Engine doesn't respond
- Make sure you're typing UCI commands (not PowerShell commands)
- Type `uci` first to initialize
- Check the executable is running (Task Manager)

### Search is slow
- This is normal! Depth 6+ can take minutes
- Try lower depths first (depth 3-4)
- Use ClassicEval for faster search: `setoption name Use NNUE value false`

### Can't find network file
- Use absolute path: `C:\Users\horne\projects\chess_engine\chess\nnue_weights.bin`
- Or copy `nnue_weights.bin` to the same folder as `chess_engine.exe`

## Quick Test Script

Save this as `test.ps1`:

```powershell
cd build\Release
$commands = @"
uci
isready
position startpos
go depth 3
quit
"@
$commands | .\chess_engine.exe
```

Run with: `.\test.ps1`

