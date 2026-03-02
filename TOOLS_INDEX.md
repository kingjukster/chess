# Development Tools Index

Quick navigation to all development tools and documentation.

## 📚 Documentation

| Document | Description | Location |
|----------|-------------|----------|
| **Development Tools Guide** | Complete guide to all tools (400+ lines) | [DEVELOPMENT_TOOLS.md](DEVELOPMENT_TOOLS.md) |
| **Quick Reference** | Fast reference for common commands | [TOOLS_QUICK_REFERENCE.md](TOOLS_QUICK_REFERENCE.md) |
| **Python Tools Guide** | Guide for play_games.py and analyze.py | [tools/README.md](tools/README.md) |
| **Fuzzing Guide** | Complete fuzzing documentation | [fuzz/README.md](fuzz/README.md) |
| **Tools Summary** | Overview of what was created | [../DEVELOPMENT_TOOLS_SUMMARY.md](../DEVELOPMENT_TOOLS_SUMMARY.md) |

## 🔧 Tools

### Benchmarking
- **Location:** `bench/`
- **Usage:** `bench [depth]` in UCI
- **Purpose:** Performance measurement and verification
- **Docs:** [DEVELOPMENT_TOOLS.md#benchmarking-suite](DEVELOPMENT_TOOLS.md#benchmarking-suite)

### EPD Testing
- **Location:** `epd/`
- **Files:** `wac.epd`, `arasan.epd`, `sts.epd`
- **Purpose:** Tactical and positional strength testing
- **Docs:** [DEVELOPMENT_TOOLS.md#epd-test-suite](DEVELOPMENT_TOOLS.md#epd-test-suite)

### Game Player
- **Location:** `tools/play_games.py`
- **Usage:** `python3 tools/play_games.py engine1 engine2 -g 10 -t 60`
- **Purpose:** Automated game playing and ELO estimation
- **Docs:** [tools/README.md#play_gamespy](tools/README.md#play_gamespy)

### Position Analyzer
- **Location:** `tools/analyze.py`
- **Usage:** `python3 tools/analyze.py engine -f "FEN" -d 20 -m 3`
- **Purpose:** Position analysis and visualization
- **Docs:** [tools/README.md#analyzepy](tools/README.md#analyzepy)

### Debug Tools
- **Commands:** `d`, `setoption name Debug value true`
- **Purpose:** Position display and debug output
- **Docs:** [DEVELOPMENT_TOOLS.md#debug-tools](DEVELOPMENT_TOOLS.md#debug-tools)

### Profiling
- **CMake Options:** `ENABLE_PROFILING`, `ENABLE_GPROF`, `ENABLE_SANITIZERS`
- **Purpose:** Performance profiling and optimization
- **Docs:** [DEVELOPMENT_TOOLS.md#profiling](DEVELOPMENT_TOOLS.md#profiling)

### Fuzzing
- **Location:** `fuzz/`
- **Files:** `fuzz_movegen.cpp`, `fuzz_fen.cpp`
- **Purpose:** Automated bug finding
- **Docs:** [fuzz/README.md](fuzz/README.md)

## 🚀 Quick Start

### 1. Build the Engine
```bash
cd chess/build
cmake ..
make -j4
```

### 2. Run Benchmark
```bash
./chess_engine
bench 13
quit
```

### 3. Test Position Display
```bash
./chess_engine
d
quit
```

### 4. Analyze a Position
```bash
python3 ../tools/analyze.py ./chess_engine
```

### 5. Play Games
```bash
python3 ../tools/play_games.py ./chess_engine ./chess_engine -g 2 -d 5
```

## 📖 Common Tasks

### Measure Performance
```bash
# Benchmark
./chess_engine
bench 13

# Profile
cmake .. -DENABLE_GPROF=ON
make
./chess_engine
bench 13
quit
gprof chess_engine gmon.out
```

### Test Strength
```cpp
// EPD tests
EpdRunner runner(&search);
runner.run_suite("epd/wac.epd", 10);

// Play games
python3 tools/play_games.py engine1 engine2 -g 100 -t 60
```

### Debug Issues
```bash
# Display position
./chess_engine
position fen "..."
d

# Enable debug mode
setoption name Debug value true

# Run with sanitizers
cmake .. -DENABLE_SANITIZERS=ON
make
./chess_engine
```

### Find Bugs
```bash
# Build fuzzers
cd fuzz
./build_fuzzers.sh

# Run fuzzing
mkdir corpus_fen
./fuzz_fen corpus_fen -max_total_time=3600
```

## 🎯 Workflows

### Development Workflow
1. Write code
2. Build: `make`
3. Test: `./chess_tests`
4. Benchmark: `bench 13`
5. Profile if needed
6. Commit changes

### Testing Workflow
1. Run EPD tests
2. Play games vs baseline
3. Check for regressions
4. Verify improvements

### Optimization Workflow
1. Profile to find bottlenecks
2. Optimize hot functions
3. Benchmark to verify improvement
4. Test for regressions

### Release Workflow
1. Run all tests
2. Run benchmark
3. Play games vs previous version
4. Verify no regressions
5. Tag release

## 📊 Performance Targets

### Benchmark (depth 13)
- Nodes: ~50M+
- Time: <60s
- NPS: >1M
- Signature: Consistent across builds

### EPD Tests
- WAC: >80% pass rate (ELO ~2000)
- Arasan: >70% pass rate (ELO ~2100)
- STS: >65% pass rate (ELO ~2000)

### Game Playing
- Win rate vs self: ~50%
- Average game length: 40-80 moves
- Timeouts: <1%

## 🐛 Troubleshooting

| Issue | Solution | Reference |
|-------|----------|-----------|
| Benchmark signature mismatch | Check for non-deterministic behavior | [DEVELOPMENT_TOOLS.md](DEVELOPMENT_TOOLS.md#signature-verification) |
| EPD tests failing | Increase depth or time limit | [DEVELOPMENT_TOOLS.md](DEVELOPMENT_TOOLS.md#running-tests) |
| Profiling no data | Enable profiling flags in CMake | [DEVELOPMENT_TOOLS.md](DEVELOPMENT_TOOLS.md#profiling) |
| Fuzzer crashes | Minimize input and fix bug | [fuzz/README.md](fuzz/README.md#analyzing-crashes) |
| Games timeout | Reduce time or use fixed depth | [tools/README.md](tools/README.md#troubleshooting) |
| Analysis too slow | Reduce depth or MultiPV | [tools/README.md](tools/README.md#troubleshooting) |

## 🔗 Related Documentation

- [Main README](../README.md) - Project overview
- [HOW_TO_RUN.md](HOW_TO_RUN.md) - Build and run instructions
- [TESTING.md](TESTING.md) - Testing guide
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines

## 📝 File Structure

```
chess/
├── bench/                      # Benchmarking suite
│   ├── bench.h
│   ├── bench.cpp
│   └── positions.txt
├── epd/                        # EPD test suites
│   ├── epd_parser.h
│   ├── epd_parser.cpp
│   ├── epd_runner.h
│   ├── epd_runner.cpp
│   ├── wac.epd
│   ├── arasan.epd
│   └── sts.epd
├── tools/                      # Python tools
│   ├── play_games.py
│   ├── analyze.py
│   └── README.md
├── fuzz/                       # Fuzzing harnesses
│   ├── fuzz_movegen.cpp
│   ├── fuzz_fen.cpp
│   ├── build_fuzzers.sh
│   └── README.md
├── DEVELOPMENT_TOOLS.md        # Complete guide
├── TOOLS_QUICK_REFERENCE.md    # Quick reference
└── TOOLS_INDEX.md             # This file
```

## 💡 Tips

1. **Start with benchmarks** - Establish baseline performance
2. **Use EPD tests** - Measure tactical/positional strength
3. **Profile regularly** - Find and fix bottlenecks
4. **Fuzz continuously** - Catch bugs early
5. **Document changes** - Keep track of improvements
6. **Test regressions** - Verify changes don't break things
7. **Automate testing** - Create scripts for common tasks

## 🎓 Learning Resources

- [Chess Programming Wiki](https://www.chessprogramming.org/)
- [libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [Perf Tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)
- [UCI Protocol](https://www.shredderchess.com/chess-info/features/uci-universal-chess-interface.html)

---

**Everything you need to develop, test, and optimize the chess engine!**
