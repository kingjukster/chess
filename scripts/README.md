# Scripts Directory

This directory contains utility scripts for benchmarking, testing, and CI/CD operations.

## Benchmarking Scripts

### `benchmark.py` (Recommended)

Comprehensive Python benchmark script with detailed output and JSON export.

**Features:**
- Standard benchmark positions (6 positions)
- Perft verification tests
- Nodes per second (NPS) calculation
- JSON output for result tracking
- Historical comparison support

**Usage:**
```bash
# Basic benchmark
python3 benchmark.py --engine ../build/chess_engine

# With JSON output
python3 benchmark.py --engine ../build/chess_engine --output results.json

# Include perft tests
python3 benchmark.py --engine ../build/chess_engine --perft

# Verbose output
python3 benchmark.py --engine ../build/chess_engine --verbose
```

**Requirements:**
- Python 3.7+
- No external dependencies

### `benchmark.sh` (Unix/Linux/macOS)

Shell script for quick benchmarks without Python dependency.

**Features:**
- Standard benchmark positions
- Perft tests
- Colored output
- Fast execution

**Usage:**
```bash
# Make executable (first time only)
chmod +x benchmark.sh

# Run benchmark
./benchmark.sh --engine ../build/chess_engine

# Custom depth
./benchmark.sh --engine ../build/chess_engine --depth 10
```

**Requirements:**
- Bash shell
- Basic Unix utilities (grep, sed, bc)

### `benchmark.bat` (Windows)

Windows batch script for benchmarking.

**Features:**
- Standard benchmark positions
- Perft tests
- Windows-compatible

**Usage:**
```cmd
REM Run benchmark
benchmark.bat --engine ..\build\Release\chess_engine.exe

REM Custom depth
benchmark.bat --engine ..\build\Release\chess_engine.exe --depth 10
```

**Requirements:**
- Windows command prompt
- No external dependencies

### `compare_benchmarks.py`

Compare two benchmark results and detect performance regressions.

**Features:**
- Side-by-side comparison
- Percentage change calculation
- Regression detection (configurable threshold)
- Per-position analysis
- Perft comparison

**Usage:**
```bash
# Compare two results
python3 compare_benchmarks.py old_results.json new_results.json

# Custom regression threshold (default: 5%)
python3 compare_benchmarks.py old.json new.json --threshold 0.10

# Fail on regression (for CI)
python3 compare_benchmarks.py old.json new.json --fail-on-regression
```

**Output Example:**
```
================================================================================
Benchmark Comparison
================================================================================

Overall Performance:
  Previous: 1,234,567 nps
  Current:  1,345,678 nps
  Change:   +9.00%
  ✓ IMPROVEMENT (>5% faster)

Position-by-Position Comparison:
--------------------------------------------------------------------------------
Position             Old NPS         New NPS         Change     Status
--------------------------------------------------------------------------------
startpos             1,200,000       1,320,000       +10.00%  ✓ FASTER
kiwipete             1,150,000       1,140,000        -0.87%  ✓ STABLE
...
```

## Testing Scripts

### `test_ci_locally.sh`

Run the same checks that CI will run, locally before pushing.

**Features:**
- Build verification (Debug + Release)
- Perft tests
- UCI protocol tests
- Code formatting check
- Static analysis (if clang-tidy available)
- Performance benchmark
- Comprehensive summary

**Usage:**
```bash
# Make executable (first time only)
chmod +x test_ci_locally.sh

# Run all checks
./test_ci_locally.sh
```

**Requirements:**
- CMake
- Ninja (or other build system)
- clang-format (optional, for formatting checks)
- clang-tidy (optional, for static analysis)
- Python 3 (for benchmark)

**Output:**
```
========================================================================
Local CI Test Runner
========================================================================

Running: Build (Debug)
✓ Build (Debug) passed

Running: Build (Release)
✓ Build (Release) passed

Running: Perft Tests
✓ Perft Tests passed

Running: UCI Protocol Tests
✓ UCI Protocol Tests passed

...

========================================================================
Summary
========================================================================
✓ All tests passed! Ready to push.
```

### `setup_scripts.sh`

Make all scripts executable (Unix/Linux/macOS only).

**Usage:**
```bash
# Run once after cloning
chmod +x setup_scripts.sh
./setup_scripts.sh
```

This will make all `.sh` and `.py` scripts executable.

## Benchmark Positions

The benchmark suite uses 6 standard positions:

1. **startpos** (depth 8)
   - Starting position
   - Tests basic move generation and search

2. **kiwipete** (depth 7)
   - Complex middlegame position
   - Tests castling, en passant, promotions
   - FEN: `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`

3. **endgame** (depth 10)
   - Complex endgame
   - Tests deep search in reduced material
   - FEN: `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`

4. **tactics** (depth 7)
   - Tactical position with many captures
   - Tests quiescence search
   - FEN: `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1`

5. **promotion** (depth 7)
   - Position with promotion threats
   - Tests promotion handling
   - FEN: `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8`

6. **middlegame** (depth 7)
   - Balanced middlegame
   - Tests typical middlegame evaluation
   - FEN: `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10`

## Perft Tests

Standard perft tests verify move generation correctness:

| Depth | Expected Nodes |
|-------|----------------|
| 1     | 20             |
| 2     | 400            |
| 3     | 8,902          |
| 4     | 197,281        |
| 5     | 4,865,609      |
| 6     | 119,060,324    |

## CI/CD Integration

These scripts are used by GitHub Actions workflows:

- **CI Workflow**: Uses `benchmark.py` and `compare_benchmarks.py`
- **Local Testing**: Use `test_ci_locally.sh` before pushing
- **Release Workflow**: Builds are tested with perft before release

See `.github/CI_CD_GUIDE.md` for more information.

## Troubleshooting

### "Permission denied" error (Unix/Linux/macOS)

Make scripts executable:
```bash
chmod +x *.sh *.py
```

Or use the setup script:
```bash
chmod +x setup_scripts.sh && ./setup_scripts.sh
```

### Python scripts don't run

Ensure Python 3.7+ is installed:
```bash
python3 --version
```

Make script executable:
```bash
chmod +x benchmark.py
```

### Benchmark times out

Reduce search depth:
```bash
./benchmark.sh --depth 6
```

Or increase timeout in the script (edit `benchmark.py`, line with `timeout=300`).

### Engine not found

Specify full path to engine:
```bash
./benchmark.sh --engine /full/path/to/chess_engine
```

Or build the engine first:
```bash
cd .. && mkdir build && cd build && cmake .. && make
```

## Contributing

When adding new scripts:

1. Add documentation to this README
2. Make scripts executable (Unix)
3. Test on multiple platforms if possible
4. Follow existing naming conventions
5. Add error handling and help messages

## License

These scripts are part of the Chess Engine project and are licensed under the MIT License.
