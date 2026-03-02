# Chess Engine Development Tools

Python scripts for testing, analyzing, and evaluating the chess engine.

## Quick Start

### Prerequisites

```bash
pip install python3
```

### Tools Overview

| Tool | Purpose | Usage |
|------|---------|-------|
| `play_games.py` | Play games between engines | `python3 play_games.py engine1 engine2 -g 10 -t 60` |
| `analyze.py` | Analyze positions | `python3 analyze.py engine -f "FEN" -d 20 -m 3` |

## play_games.py

Automated game playing for engine testing and ELO estimation.

### Basic Usage

```bash
# Play 10 games with 60 seconds per side
python3 play_games.py ../build/chess_engine opponent_engine -g 10 -t 60

# Play with fixed depth
python3 play_games.py ../build/chess_engine opponent_engine -g 20 -d 10

# Tournament with 3 engines
python3 play_games.py engine1 engine2 engine3 -g 10 -t 30
```

### Options

```
positional arguments:
  engines               Paths to engine executables

optional arguments:
  -h, --help            Show help message
  -g, --games N         Games per pairing (default: 10)
  -t, --time N          Time control in seconds per side
  -d, --depth N         Fixed search depth
  -o, --output FILE     Output PGN file (default: games.pgn)
  -r, --results FILE    Output results JSON (default: results.json)
```

### Example Output

```
Starting tournament with 2 engines
Games per pairing: 10
Total games: 20
================================================================================

Game 1/20: ChessEngine vs Opponent
Result: 1-0 - White wins
Moves: 45

...

================================================================================
TOURNAMENT RESULTS
================================================================================
Engine               Score      W-L-D           Win%
--------------------------------------------------------------------------------
ChessEngine          14.5/20    12-3-5         60.0%
Opponent             5.5/20     3-12-5         15.0%
================================================================================

Games saved to games.pgn
Results saved to results.json
```

### Output Files

**games.pgn** - All games in PGN format:
```pgn
[Event "Engine Match"]
[Date "2026.03.01"]
[White "ChessEngine"]
[Black "Opponent"]
[Result "1-0"]

1. e4 e5 2. Nf3 Nc6 ... 1-0
```

**results.json** - Tournament statistics:
```json
{
  "timestamp": "2026-03-01T12:00:00",
  "games_per_pair": 10,
  "results": {
    "ChessEngine": {"wins": 12, "losses": 3, "draws": 5}
  }
}
```

## analyze.py

Detailed position analysis with visualization and evaluation breakdown.

### Basic Usage

```bash
# Analyze starting position
python3 analyze.py ../build/chess_engine

# Analyze custom position
python3 analyze.py ../build/chess_engine \
  -f "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"

# Deep analysis with 5 variations
python3 analyze.py ../build/chess_engine -f "..." -d 25 -m 5

# Export to JSON
python3 analyze.py ../build/chess_engine -f "..." --json analysis.json

# Export to text file
python3 analyze.py ../build/chess_engine -f "..." --text analysis.txt
```

### Options

```
positional arguments:
  engine                Path to chess engine executable

optional arguments:
  -h, --help            Show help message
  -f, --fen FEN         FEN string to analyze (default: starting position)
  -d, --depth N         Search depth (default: 20)
  -m, --multipv N       Number of principal variations (default: 3)
  --json FILE           Export analysis to JSON file
  --text FILE           Export analysis to text file
  --no-display          Don't display board visualization
```

### Example Output

```
================================================================================
POSITION ANALYSIS
================================================================================

  +---+---+---+---+---+---+---+---+
8 | r |   | b | q | k | b | n | r |
  +---+---+---+---+---+---+---+---+
7 | p | p | p | p |   | p | p | p |
  ...
    a   b   c   d   e   f   g   h

FEN: r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3
Side to move: Black
Evaluation: +45 (+0.45)

Analyzing position (depth=20, multipv=3)...
================================================================================
ANALYSIS RESULTS
================================================================================
Depth: 20
Nodes: 12,345,678
Time: 5432 ms
NPS: 2,272,727

Principal Variations:
--------------------------------------------------------------------------------
 1. [  +0.45] a6 Ba4 Nf6 O-O Be7 Re1 b5 Bb3 d6 c3
 2. [  +0.38] Nf6 O-O Nxe4 d4 Nd6 Bxc6 dxc6 dxe5 Nf5
 3. [  +0.32] Qe7 O-O Nf6 Re1 O-O c3 d6 d4 Bg4
================================================================================
```

### JSON Export

```json
{
  "depth": 20,
  "nodes": 12345678,
  "time": 5432,
  "nps": 2272727,
  "best_move": "a6",
  "pv_lines": [
    {
      "num": 1,
      "score": 0.45,
      "pv": ["a6", "Ba4", "Nf6", "O-O", "Be7", "Re1"]
    }
  ]
}
```

## Common Workflows

### Testing Engine Strength

```bash
# 1. Run EPD test suites
cd ..
./chess_engine
# In UCI: load EPD positions and test

# 2. Play games against known engines
python3 tools/play_games.py chess_engine stockfish -g 100 -t 60

# 3. Analyze results
# Check win rate, average game length, etc.
```

### Analyzing Specific Positions

```bash
# Analyze a tactical position
python3 tools/analyze.py chess_engine \
  -f "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -" \
  -d 25 -m 5 --text tactical_analysis.txt

# Compare with another engine
python3 tools/analyze.py other_engine \
  -f "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -" \
  -d 25 -m 5 --text other_analysis.txt

# Compare the outputs
diff tactical_analysis.txt other_analysis.txt
```

### Regression Testing

```bash
# Before changes
python3 tools/play_games.py chess_engine_old chess_engine_old -g 1 -d 10 > /dev/null
cp games.pgn baseline_games.pgn

# After changes
python3 tools/play_games.py chess_engine_new chess_engine_old -g 100 -t 60

# Analyze results
# If new engine scores > 55%, it's likely an improvement
# If new engine scores < 45%, it's likely a regression
```

### Performance Comparison

```bash
# Test both engines on same positions
for fen in $(cat test_positions.txt); do
    echo "Testing: $fen"
    
    # Engine 1
    python3 tools/analyze.py engine1 -f "$fen" -d 15 --json e1.json
    
    # Engine 2
    python3 tools/analyze.py engine2 -f "$fen" -d 15 --json e2.json
    
    # Compare NPS, depth reached, etc.
done
```

## Troubleshooting

### Engine Not Found

```bash
# Make sure engine is built
cd ../build
cmake ..
make

# Use absolute path
python3 tools/analyze.py /full/path/to/chess_engine
```

### Engine Doesn't Respond

- Check that engine supports UCI protocol
- Try running engine manually to verify it works
- Check for error messages in stderr

### Games Timeout

- Reduce time control: `-t 10` instead of `-t 60`
- Use fixed depth: `-d 8` instead of time control
- Check engine for infinite loops

### Analysis Takes Too Long

- Reduce depth: `-d 15` instead of `-d 25`
- Reduce MultiPV: `-m 1` instead of `-m 5`
- Use faster hardware

## Tips

1. **Start small** - Test with a few games before running large tournaments
2. **Use version control** - Save baseline results for comparison
3. **Automate testing** - Create shell scripts for common test scenarios
4. **Monitor resources** - Watch CPU and memory usage during long tests
5. **Keep logs** - Save all output for later analysis

## See Also

- [DEVELOPMENT_TOOLS.md](../DEVELOPMENT_TOOLS.md) - Complete development tools guide
- [README.md](../README.md) - Main project documentation
- [TESTING.md](../TESTING.md) - Testing guide
