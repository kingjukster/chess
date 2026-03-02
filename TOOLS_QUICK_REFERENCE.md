# Development Tools Quick Reference

Fast reference for all development tools and utilities.

## Benchmarking

```bash
# Run benchmark
./chess_engine
bench [depth]

# Example
bench 13
```

**Output:** Nodes, time, NPS, signature

---

## EPD Testing

```cpp
#include "epd/epd_runner.h"

EpdRunner runner(&search);
EpdTestResult result = runner.run_suite("epd/wac.epd", 10, 5000);
EpdRunner::print_results(result);
```

**Test Suites:**
- `epd/wac.epd` - Win At Chess (tactical)
- `epd/arasan.epd` - Arasan (mixed)
- `epd/sts.epd` - Strategic Test Suite

---

## Game Playing

```bash
# Play games
python3 tools/play_games.py engine1 engine2 -g 10 -t 60

# Options
-g N    # Games per pairing
-t N    # Time in seconds
-d N    # Fixed depth
-o FILE # Output PGN
-r FILE # Results JSON
```

---

## Position Analysis

```bash
# Analyze position
python3 tools/analyze.py engine -f "FEN" -d 20 -m 3

# Options
-f FEN      # FEN string
-d N        # Depth
-m N        # MultiPV
--json FILE # Export JSON
--text FILE # Export text
```

---

## Debug Commands

```bash
# Display position
./chess_engine
d

# Enable debug mode
setoption name Debug value true
```

---

## Profiling

### Build with Profiling

```bash
# gprof
cmake .. -DENABLE_GPROF=ON
make
./chess_engine
gprof chess_engine gmon.out > profile.txt

# perf (Linux)
cmake .. -DENABLE_PROFILING=ON
make
perf record -g ./chess_engine
perf report

# Sanitizers
cmake .. -DENABLE_SANITIZERS=ON
make
./chess_engine  # Will report errors
```

---

## Fuzzing

```bash
# Build fuzzers
cd fuzz
./build_fuzzers.sh

# Run
mkdir corpus_movegen corpus_fen
./fuzz_movegen corpus_movegen -max_total_time=3600
./fuzz_fen corpus_fen -max_total_time=3600

# Analyze crash
./fuzz_movegen crash-<hash>
gdb --args ./fuzz_movegen crash-<hash>
```

---

## CMake Options

```bash
cmake .. [OPTIONS]

# Profiling
-DENABLE_PROFILING=ON    # Debug symbols, frame pointers
-DENABLE_GPROF=ON        # gprof profiling
-DENABLE_SANITIZERS=ON   # Address + UB sanitizers
-DENABLE_COVERAGE=ON     # Code coverage

# Features
-DBUILD_TESTS=ON         # Build unit tests
-DUSE_AVX2=ON           # AVX2 SIMD
-DUSE_SSE41=ON          # SSE4.1 SIMD
```

---

## UCI Commands

```
uci                     # Initialize
isready                 # Check ready
ucinewgame             # New game
position startpos       # Set starting position
position fen <FEN>      # Set FEN position
go depth <N>           # Search to depth
go movetime <ms>       # Search for time
bench [depth]          # Run benchmark
d                      # Display position
perft <depth>          # Perft test
quit                   # Exit
```

---

## File Structure

```
chess/
├── bench/              # Benchmarking
│   ├── bench.cpp
│   └── positions.txt
├── epd/               # EPD test suites
│   ├── epd_parser.cpp
│   ├── epd_runner.cpp
│   ├── wac.epd
│   ├── arasan.epd
│   └── sts.epd
├── tools/             # Python tools
│   ├── play_games.py
│   └── analyze.py
├── fuzz/              # Fuzzing harnesses
│   ├── fuzz_movegen.cpp
│   ├── fuzz_fen.cpp
│   └── build_fuzzers.sh
└── DEVELOPMENT_TOOLS.md
```

---

## Common Workflows

### Test Engine Strength

```bash
# 1. Benchmark
./chess_engine
bench 13

# 2. EPD tests
# (Run from C++ code)

# 3. Play games
python3 tools/play_games.py chess_engine opponent -g 100 -t 60
```

### Profile Performance

```bash
# 1. Build with profiling
cmake .. -DENABLE_GPROF=ON
make

# 2. Run benchmark
./chess_engine
bench 13
quit

# 3. Analyze
gprof chess_engine gmon.out | less
```

### Find Bugs

```bash
# 1. Build with sanitizers
cmake .. -DENABLE_SANITIZERS=ON
make

# 2. Run tests
./chess_tests

# 3. Run fuzzer
cd fuzz
./build_fuzzers.sh
./fuzz_movegen corpus_movegen
```

### Regression Test

```bash
# Before changes
bench 13 > baseline.txt

# Make changes
# ...

# After changes
bench 13 > new.txt
diff baseline.txt new.txt
```

---

## Performance Targets

### Benchmark (depth 13)
- **Nodes:** ~50M+
- **Time:** <60s
- **NPS:** >1M

### EPD Tests
- **WAC:** >80% pass rate
- **Arasan:** >70% pass rate
- **STS:** >65% pass rate

### Game Playing
- **Win rate vs self:** ~50%
- **Average game length:** 40-80 moves
- **Timeouts:** <1%

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Benchmark signature mismatch | Check for non-deterministic behavior |
| EPD tests failing | Increase depth or time limit |
| Profiling shows no data | Ensure profiling flags enabled |
| Fuzzer finds crashes | Minimize input, fix bug, re-run |
| Games timeout | Reduce time control or use fixed depth |
| Analysis too slow | Reduce depth or MultiPV count |

---

## Resources

- **Full Documentation:** [DEVELOPMENT_TOOLS.md](DEVELOPMENT_TOOLS.md)
- **Tools README:** [tools/README.md](tools/README.md)
- **Fuzzing Guide:** [fuzz/README.md](fuzz/README.md)
- **Main README:** [README.md](README.md)
