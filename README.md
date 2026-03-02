# Chess Engine with NNUE-First Architecture

[![CI](https://github.com/YOUR_USERNAME/YOUR_REPO/workflows/CI/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/ci.yml)
[![Release](https://github.com/YOUR_USERNAME/YOUR_REPO/workflows/Release/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/release.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A modern chess engine built with NNUE (Efficiently Updatable Neural Networks) evaluation in mind from the start.

## Architecture

The engine is organized into clean, modular components:

- **board/**: Position representation, move encoding, make/unmake with NNUE hooks
- **movegen/**: Attack tables, magic bitboards, move generation, perft
- **search/**: Alpha-beta search, quiescence, transposition table, move ordering
- **eval/**: Evaluation interface with pluggable backends
- **nnue/**: NNUE evaluator with HalfKP features and incremental accumulator updates
- **uci/**: UCI protocol implementation

## Key Design Decisions

### NNUE-First Architecture

1. **HalfKP Feature Set**: Features depend on king square + piece square (and piece type/color)
2. **Incremental Accumulator Updates**: Evaluation accumulator is updated incrementally on make/unmake, not recomputed
3. **Evaluation Hooks**: `make_move()` and `unmake_move()` call evaluator hooks for incremental updates
4. **UndoInfo with NNUE Deltas**: Stores all information needed for incremental NNUE updates

### Build Order

1. ✅ Position + Move + Make/Unmake (with hooks)
2. ✅ Movegen + Perft correctness
3. ✅ Search + TT + UCI
4. ✅ NNUE integration (placeholder - ready for network weights)
5. ⏳ Strength work (tuning, pruning improvements)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Run the engine:
```bash
./chess_engine
```

The engine supports UCI protocol. Connect with any UCI-compatible chess GUI.

### UCI Commands

- `uci` - Initialize UCI
- `isready` - Check if engine is ready
- `ucinewgame` - Start new game
- `position startpos` - Set position to starting position
- `position fen <fen>` - Set position from FEN
- `go depth <n>` - Search to depth n
- `go wtime <ms> btime <ms>` - Search with time controls
- `perft <depth>` - Run perft test

### Switching Evaluators

Use UCI option to switch between ClassicEval and NnueEval:
```
setoption name Use NNUE value true
```

## Current Status

- ✅ Classic evaluation (PST + material)
- ✅ Move generation with magic bitboards
- ✅ Alpha-beta search with quiescence
- ✅ Transposition table
- ✅ UCI protocol
- ✅ NNUE framework (ready for network weights)
- ⏳ Network weights loading
- ⏳ Advanced pruning (razoring, futility)
- ⏳ Move ordering improvements
- ⏳ Time management improvements

## Testing

Run perft tests to verify move generation correctness:
```
perft 1
perft 2
perft 3
```

Expected results:
- Depth 1: 20 moves
- Depth 2: 400 moves
- Depth 3: 8902 moves

### Automated Testing

The project includes comprehensive CI/CD pipelines:

- **Continuous Integration**: Automatic builds and tests on every push
  - Multi-platform builds (Linux, Windows, macOS)
  - Multiple compilers (GCC, Clang, MSVC)
  - Debug and Release configurations
  - Perft verification tests
  - UCI protocol tests
  - Performance benchmarking
  - Code coverage reporting

- **Automated Releases**: Binary releases for all platforms
  - Linux (x86_64)
  - Windows (x86_64)
  - macOS (Universal binary: x86_64 + ARM64)

### Benchmarking

Run performance benchmarks:

```bash
# Python version (detailed)
python3 scripts/benchmark.py --engine ./build/chess_engine --output results.json

# Shell version (quick)
./scripts/benchmark.sh --engine ./build/chess_engine --depth 8

# With perft tests
python3 scripts/benchmark.py --engine ./build/chess_engine --perft
```

Compare benchmark results:

```bash
python3 scripts/compare_benchmarks.py old_results.json new_results.json
```

## CI/CD Pipeline

This project includes a comprehensive CI/CD pipeline with:

- **Multi-platform builds** (Linux, Windows, macOS)
- **Multiple compilers** (GCC, Clang, MSVC)
- **Automated testing** (perft, UCI protocol)
- **Performance benchmarking** with regression detection
- **Code coverage** reporting
- **Static analysis** and formatting checks
- **Automated releases** with binaries for all platforms

**Quick Setup**: See [QUICK_CI_SETUP.md](QUICK_CI_SETUP.md) for 5-minute setup guide

**Full Documentation**: See [.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md) for complete CI/CD documentation

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

Before submitting a pull request:
1. Run local tests: `./scripts/test_ci_locally.sh`
2. Check formatting: `clang-format -i **/*.cpp **/*.h`
3. Run benchmarks: `python3 scripts/benchmark.py --engine ./build/chess_engine`

## Next Steps

1. Load actual NNUE network weights
2. Implement proper network forward pass with SIMD
3. Add advanced search techniques (razoring, futility pruning)
4. Tune evaluation and search parameters
5. Add opening book and endgame tablebase support

