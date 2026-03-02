# Development Tools and Utilities

Comprehensive guide to the development tools and utilities for the chess engine.

## Table of Contents

1. [Benchmarking Suite](#benchmarking-suite)
2. [EPD Test Suite](#epd-test-suite)
3. [Game Player](#game-player)
4. [Position Analyzer](#position-analyzer)
5. [Debug Tools](#debug-tools)
6. [Profiling](#profiling)
7. [Fuzzing](#fuzzing)

---

## Benchmarking Suite

The benchmarking suite measures engine performance across a variety of positions.

### Location

- `bench/` - Benchmark implementation
- `bench/positions.txt` - Standard benchmark positions

### Usage

#### From UCI Interface

```bash
./chess_engine
bench [depth]
```

**Example:**
```bash
bench 13
```

This will:
- Search 40+ standard positions to the specified depth
- Report nodes searched, time taken, and NPS (nodes per second)
- Generate a signature (XOR of node counts) for build verification

#### From C++ Code

```cpp
#include "bench/bench.h"

Search search(&evaluator);
Benchmark bench(&search);

// Run default benchmark at depth 13
BenchResult result = bench.run(13);
Benchmark::print_results(result);

// Run custom positions
std::vector<BenchPosition> positions = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Start"},
    {"r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", "Italian"}
};
BenchResult custom = bench.run_custom(positions, 15);
```

### Output Format

```
Running benchmark with 40 positions at depth 13
================================================================================
Position 1/40: Starting Position
FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
  Nodes: 1234567
  Time: 1234 ms
  NPS: 1000000
  Best move: e2e4

...

================================================================================
BENCHMARK RESULTS
================================================================================
Total nodes: 50000000
Total time: 45000 ms
Nodes per second: 1111111
Signature: 0xABCDEF123456
================================================================================
```

### Signature Verification

The signature (XOR of all node counts) allows you to verify that different builds produce identical search results:

```bash
# Build 1
bench 13
# Signature: 0xABCDEF123456

# Build 2 (after changes)
bench 13
# Signature: 0xABCDEF123456  ✓ Same = deterministic
```

### Adding Custom Positions

Edit `bench/positions.txt`:

```
# Format: FEN | Description
r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3 | Italian Game
```

---

## EPD Test Suite

EPD (Extended Position Description) test suites evaluate tactical and positional strength.

### Location

- `epd/` - EPD parser and test runner
- `epd/wac.epd` - Win At Chess (tactical puzzles)
- `epd/arasan.epd` - Arasan test suite
- `epd/sts.epd` - Strategic Test Suite

### EPD Format

```
FEN bm <best_move>; id "position_id"; c0 "description";
```

**Example:**
```
2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - bm Qg6; id "WAC.001";
```

### Running Tests

#### From Command Line

```bash
# Create test runner (Python script)
python3 tools/run_epd_tests.py chess_engine epd/wac.epd --depth 10
```

#### From C++ Code

```cpp
#include "epd/epd_runner.h"

Search search(&evaluator);
EpdRunner runner(&search);

// Run test suite
EpdTestResult result = runner.run_suite("epd/wac.epd", 10, 5000);
EpdRunner::print_results(result);

// Estimate ELO
int elo = EpdRunner::estimate_elo(result.pass_rate, "WAC");
std::cout << "Estimated ELO: " << elo << std::endl;
```

### Output Format

```
Loading EPD test suite: epd/wac.epd
Loaded 300 positions
Testing with depth=10, time_limit=5000ms
================================================================================
Position 1/300 [WAC.001] ✓ PASS - Tactical puzzle
Position 2/300 [WAC.002] ✗ FAIL - Rook endgame
...
================================================================================
EPD TEST RESULTS
================================================================================
Total positions: 300
Passed: 245
Failed: 55
Pass rate: 81.7%
Total time: 1234567 ms

Failed positions:
  - WAC.002
  - WAC.015
  ...
================================================================================
```

### Test Suites

#### Win At Chess (WAC)
- **Type:** Tactical puzzles
- **Positions:** ~300
- **Focus:** Tactics, combinations, forcing moves
- **ELO Correlation:** 60% = 1600, 80% = 2000, 95% = 2400

#### Arasan
- **Type:** Mixed (tactical + positional)
- **Positions:** Variable
- **Focus:** General chess strength
- **ELO Correlation:** 60% = 1900, 80% = 2300, 90% = 2500

#### Strategic Test Suite (STS)
- **Type:** Positional understanding
- **Positions:** ~1500
- **Focus:** Strategy, planning, piece placement
- **ELO Correlation:** 55% = 1800, 75% = 2200, 85% = 2400

---

## Game Player

Automated game playing for engine testing and ELO estimation.

### Location

`tools/play_games.py`

### Usage

```bash
# Play 10 games between two engines
python3 tools/play_games.py engine1 engine2 -g 10 -t 60

# Play with fixed depth
python3 tools/play_games.py engine1 engine2 -g 20 -d 10

# Tournament with multiple engines
python3 tools/play_games.py engine1 engine2 engine3 -g 10 -t 30
```

### Options

- `-g, --games N` - Games per pairing (default: 10)
- `-t, --time N` - Time control in seconds per side
- `-d, --depth N` - Fixed search depth
- `-o, --output FILE` - Output PGN file (default: games.pgn)
- `-r, --results FILE` - Output results JSON (default: results.json)

### Output

#### Console Output

```
Starting tournament with 2 engines
Games per pairing: 10
Total games: 20
================================================================================

Game 1/20: ChessEngine vs Opponent
Result: 1-0 - White wins
Moves: 45

Game 2/20: Opponent vs ChessEngine
Result: 1/2-1/2 - Draw by move limit
Moves: 150

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

#### PGN Output

```pgn
[Event "Engine Match"]
[Date "2026.03.01"]
[White "ChessEngine"]
[Black "Opponent"]
[Result "1-0"]
[TimeControl "60"]
[Termination "Black loses on time"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 ... 1-0
```

#### JSON Output

```json
{
  "timestamp": "2026-03-01T12:00:00",
  "games_per_pair": 10,
  "time_control": 60000,
  "depth": null,
  "results": {
    "ChessEngine": {
      "wins": 12,
      "losses": 3,
      "draws": 5
    },
    "Opponent": {
      "wins": 3,
      "losses": 12,
      "draws": 5
    }
  }
}
```

### ELO Calculation

Use the results to estimate ELO difference:

```python
# Expected score formula: E = 1 / (1 + 10^((opponent_elo - your_elo) / 400))
# If you score 75% against a 2000 ELO engine:
# 0.75 = 1 / (1 + 10^((2000 - your_elo) / 400))
# your_elo ≈ 2193
```

---

## Position Analyzer

Detailed position analysis with visualization and evaluation breakdown.

### Location

`tools/analyze.py`

### Usage

```bash
# Analyze starting position
python3 tools/analyze.py chess_engine

# Analyze custom position
python3 tools/analyze.py chess_engine -f "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"

# Deep analysis with multiple lines
python3 tools/analyze.py chess_engine -f "..." -d 25 -m 5

# Export to JSON
python3 tools/analyze.py chess_engine -f "..." --json analysis.json

# Export to text file
python3 tools/analyze.py chess_engine -f "..." --text analysis.txt
```

### Options

- `-f, --fen FEN` - FEN string to analyze
- `-d, --depth N` - Search depth (default: 20)
- `-m, --multipv N` - Number of principal variations (default: 3)
- `--json FILE` - Export analysis to JSON
- `--text FILE` - Export analysis to text file
- `--no-display` - Skip board visualization

### Output

```
================================================================================
POSITION ANALYSIS
================================================================================

  +---+---+---+---+---+---+---+---+
8 | r |   | b | q | k | b | n | r |
  +---+---+---+---+---+---+---+---+
7 | p | p | p | p |   | p | p | p |
  +---+---+---+---+---+---+---+---+
6 |   |   | n |   |   |   |   |   |
  +---+---+---+---+---+---+---+---+
5 |   | B |   |   | p |   |   |   |
  +---+---+---+---+---+---+---+---+
4 |   |   |   |   | P |   |   |   |
  +---+---+---+---+---+---+---+---+
3 |   |   |   |   |   | N |   |   |
  +---+---+---+---+---+---+---+---+
2 | P | P | P | P |   | P | P | P |
  +---+---+---+---+---+---+---+---+
1 | R | N | B | Q | K |   |   | R |
  +---+---+---+---+---+---+---+---+
    a   b   c   d   e   f   g   h

FEN: r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3
Key: 0x123456789ABCDEF0
Side to move: Black
Castling: KQkq
50-move counter: 3
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

### JSON Export Format

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
      "pv": ["a6", "Ba4", "Nf6", "O-O", "Be7", "Re1", "b5", "Bb3", "d6", "c3"]
    },
    {
      "num": 2,
      "score": 0.38,
      "pv": ["Nf6", "O-O", "Nxe4", "d4", "Nd6", "Bxc6", "dxc6", "dxe5", "Nf5"]
    }
  ]
}
```

---

## Debug Tools

Built-in debugging features for development.

### Display Command (`d`)

Shows the current position in the UCI interface:

```bash
./chess_engine
position startpos moves e2e4 e7e5
d
```

**Output:**
```
  +---+---+---+---+---+---+---+---+
8 | r | n | b | q | k | b | n | r |
  +---+---+---+---+---+---+---+---+
7 | p | p | p | p |   | p | p | p |
  +---+---+---+---+---+---+---+---+
6 |   |   |   |   |   |   |   |   |
  +---+---+---+---+---+---+---+---+
5 |   |   |   |   | p |   |   |   |
  +---+---+---+---+---+---+---+---+
4 |   |   |   |   | P |   |   |   |
  +---+---+---+---+---+---+---+---+
3 |   |   |   |   |   |   |   |   |
  +---+---+---+---+---+---+---+---+
2 | P | P | P | P |   | P | P | P |
  +---+---+---+---+---+---+---+---+
1 | R | N | B | Q | K | B | N | R |
  +---+---+---+---+---+---+---+---+
    a   b   c   d   e   f   g   h

FEN: rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2
Key: 0x463b96181691fc9c
Side to move: White
Castling: KQkq
En passant: e6
50-move counter: 0
Evaluation: +35 (+0.35)
```

### Debug Mode

Enable debug output:

```bash
setoption name Debug value true
```

This enables:
- Detailed search information
- Move ordering statistics
- Transposition table hit rates
- Evaluation breakdown

---

## Profiling

Performance profiling to identify bottlenecks.

### Building with Profiling Support

#### Linux/macOS with gprof

```bash
cd chess/build
cmake .. -DENABLE_GPROF=ON
make
```

Run the engine:
```bash
./chess_engine
bench 13
quit
```

Analyze profile:
```bash
gprof chess_engine gmon.out > profile.txt
less profile.txt
```

#### Linux with perf

```bash
cd chess/build
cmake .. -DENABLE_PROFILING=ON
make

# Record profile
perf record -g ./chess_engine
# In engine: bench 13, then quit

# View report
perf report
```

#### macOS with Instruments

```bash
cd chess/build
cmake .. -DENABLE_PROFILING=ON
make

# Profile with Instruments
instruments -t "Time Profiler" ./chess_engine
```

#### Windows with Visual Studio Profiler

```bash
cd chess/build
cmake .. -DENABLE_PROFILING=ON -G "Visual Studio 17 2022"
cmake --build . --config Release

# Use Visual Studio Profiler:
# Debug -> Performance Profiler -> CPU Usage
```

### Sanitizers

Build with sanitizers to catch bugs:

```bash
# Address Sanitizer + Undefined Behavior Sanitizer
cmake .. -DENABLE_SANITIZERS=ON
make

./chess_engine
# Will report memory errors, leaks, undefined behavior
```

### Code Coverage

```bash
cmake .. -DENABLE_COVERAGE=ON
make

# Run tests
./chess_tests

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

---

## Fuzzing

Automated testing with random inputs to find bugs.

### Location

- `fuzz/` - Fuzzing harnesses
- `fuzz/fuzz_movegen.cpp` - Move generation fuzzer
- `fuzz/fuzz_fen.cpp` - FEN parsing fuzzer
- `fuzz/build_fuzzers.sh` - Build script

### Building Fuzzers

```bash
cd chess/fuzz
./build_fuzzers.sh
```

Or manually:

```bash
clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
    fuzz_movegen.cpp \
    ../board/position.cpp \
    ../movegen/movegen.cpp \
    ../movegen/attacks.cpp \
    -I.. -std=c++17 \
    -o fuzz_movegen
```

### Running Fuzzers

```bash
# Create corpus directory
mkdir corpus_movegen corpus_fen

# Seed with valid inputs
echo "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" > corpus_fen/start.txt

# Run fuzzer
./fuzz_movegen corpus_movegen -max_total_time=3600  # 1 hour
./fuzz_fen corpus_fen -max_total_time=3600
```

### Analyzing Crashes

When a crash is found:

```bash
# Reproduce
./fuzz_movegen crash-<hash>

# Debug
gdb --args ./fuzz_movegen crash-<hash>

# Get detailed info
ASAN_OPTIONS=symbolize=1:verbosity=1 ./fuzz_movegen crash-<hash>
```

### Continuous Fuzzing

```bash
#!/bin/bash
# Run fuzzers continuously
while true; do
    timeout 3600 ./fuzz_movegen corpus_movegen &
    timeout 3600 ./fuzz_fen corpus_fen &
    wait
    
    # Check for crashes
    if ls crash-* 1> /dev/null 2>&1; then
        echo "Crash found!"
        exit 1
    fi
    
    echo "Fuzzing cycle complete, restarting..."
done
```

---

## Best Practices

### Development Workflow

1. **Write code** with proper error handling
2. **Run unit tests** (`./chess_tests`)
3. **Run benchmarks** to check performance (`bench 13`)
4. **Run EPD tests** to check strength (`python3 tools/run_epd_tests.py`)
5. **Profile** to find bottlenecks
6. **Fuzz** to find edge cases
7. **Play games** to verify improvements

### Performance Testing

```bash
# Baseline
bench 13 > baseline.txt

# Make changes
# ...

# Compare
bench 13 > new.txt
diff baseline.txt new.txt
```

### Regression Testing

```bash
# Save current results
bench 13 > bench_baseline.txt
python3 tools/run_epd_tests.py chess_engine epd/wac.epd > wac_baseline.txt

# After changes, compare
bench 13 > bench_new.txt
python3 tools/run_epd_tests.py chess_engine epd/wac.epd > wac_new.txt

# Check for regressions
diff bench_baseline.txt bench_new.txt
diff wac_baseline.txt wac_new.txt
```

### Continuous Integration

Example CI workflow:

```yaml
- name: Build
  run: |
    cd chess/build
    cmake ..
    make -j4

- name: Test
  run: ./chess_tests

- name: Benchmark
  run: |
    ./chess_engine <<EOF
    bench 13
    quit
    EOF

- name: EPD Tests
  run: python3 tools/run_epd_tests.py chess_engine epd/wac.epd
```

---

## Troubleshooting

### Benchmark Signature Mismatch

If signatures differ between runs:
- Check for non-deterministic behavior (random number generation, threading)
- Verify transposition table is cleared between runs
- Ensure time-based search termination is disabled

### EPD Tests Failing

- Increase search depth or time limit
- Check that move parsing handles both coordinate and SAN notation
- Verify evaluator is properly initialized

### Profiling Shows No Data

- Ensure profiling flags are enabled in CMake
- Run enough work (e.g., `bench 13` instead of `bench 5`)
- Check that profiling tools are installed

### Fuzzer Finds Crashes

- Minimize the crashing input: `./fuzz_movegen -minimize_crash=1 crash-file`
- Fix the bug
- Add the minimized input to regression tests
- Re-run fuzzer to verify fix

---

## Additional Resources

- [Chess Programming Wiki](https://www.chessprogramming.org/)
- [libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [Perf Tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
