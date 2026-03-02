# Fuzzing Harnesses for Chess Engine

This directory contains fuzzing harnesses to test the robustness of the chess engine's core components.

## Overview

Fuzzing is an automated testing technique that feeds random or malformed inputs to a program to discover bugs, crashes, and security vulnerabilities. These harnesses target critical components:

1. **fuzz_movegen.cpp** - Tests move generation for crashes and correctness
2. **fuzz_fen.cpp** - Tests FEN parsing for crashes and validation

## Building with libFuzzer (Recommended)

libFuzzer is integrated into modern Clang compilers and provides excellent coverage-guided fuzzing.

### Prerequisites

- Clang 6.0 or newer
- Linux or macOS (Windows support via WSL)

### Build Commands

```bash
# Move generation fuzzer
clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
    fuzz_movegen.cpp \
    ../board/position.cpp \
    ../movegen/movegen.cpp \
    ../movegen/attacks.cpp \
    -I.. -std=c++17 \
    -o fuzz_movegen

# FEN parsing fuzzer
clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
    fuzz_fen.cpp \
    ../board/position.cpp \
    ../movegen/attacks.cpp \
    -I.. -std=c++17 \
    -o fuzz_fen
```

### Running

```bash
# Create corpus directory for interesting test cases
mkdir corpus_movegen corpus_fen

# Seed with valid FEN strings
echo "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" > corpus_fen/start.txt
echo "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3" > corpus_fen/italian.txt

# Run fuzzers
./fuzz_movegen corpus_movegen -max_total_time=3600  # Run for 1 hour
./fuzz_fen corpus_fen -max_total_time=3600
```

### Useful Options

- `-max_total_time=N` - Run for N seconds
- `-max_len=N` - Maximum input length
- `-jobs=N` - Number of parallel jobs
- `-dict=file` - Use dictionary file for better mutations
- `-timeout=N` - Timeout for single input (default: 1200s)

## Building with AFL (American Fuzzy Lop)

AFL is another popular fuzzer with different strengths.

### Prerequisites

```bash
# Install AFL
sudo apt-get install afl++  # Ubuntu/Debian
# or
brew install afl-fuzz       # macOS
```

### Build Commands

```bash
# Compile with AFL
afl-clang++ -DAFL_FUZZER -g -O1 \
    fuzz_movegen.cpp \
    ../board/position.cpp \
    ../movegen/movegen.cpp \
    ../movegen/attacks.cpp \
    -I.. -std=c++17 \
    -o fuzz_movegen_afl

afl-clang++ -DAFL_FUZZER -g -O1 \
    fuzz_fen.cpp \
    ../board/position.cpp \
    ../movegen/attacks.cpp \
    -I.. -std=c++17 \
    -o fuzz_fen_afl
```

### Running

```bash
# Create input/output directories
mkdir afl_in afl_out

# Seed with test cases
echo "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" > afl_in/start.txt

# Run AFL
afl-fuzz -i afl_in -o afl_out -- ./fuzz_fen_afl @@
```

## Creating a Seed Corpus

A good seed corpus improves fuzzing efficiency:

```bash
# Create seed corpus from EPD files
cat ../epd/*.epd | grep -v "^#" | cut -d' ' -f1-4 > corpus_fen/seeds.txt

# Add edge cases
echo "8/8/8/8/8/8/8/8 w - - 0 1" > corpus_fen/empty.txt
echo "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" > corpus_fen/start.txt
echo "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1" > corpus_fen/castling.txt
echo "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1" > corpus_fen/ep.txt
```

## Continuous Fuzzing

For continuous integration:

```bash
#!/bin/bash
# fuzz_continuous.sh

# Build fuzzers
./build_fuzzers.sh

# Run each fuzzer for 1 hour
timeout 3600 ./fuzz_movegen corpus_movegen -max_total_time=3600 &
timeout 3600 ./fuzz_fen corpus_fen -max_total_time=3600 &

wait

# Check for crashes
if [ -d "crash-*" ] || [ -d "leak-*" ] || [ -d "timeout-*" ]; then
    echo "Fuzzing found issues!"
    exit 1
fi

echo "Fuzzing completed successfully"
```

## Analyzing Crashes

When a crash is found:

```bash
# Reproduce the crash
./fuzz_movegen crash-<hash>

# Debug with GDB
gdb --args ./fuzz_movegen crash-<hash>

# Get more info with sanitizers
ASAN_OPTIONS=symbolize=1 ./fuzz_movegen crash-<hash>
```

## Integration with CMake

Add to CMakeLists.txt:

```cmake
option(BUILD_FUZZERS "Build fuzzing harnesses" OFF)

if(BUILD_FUZZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_executable(fuzz_movegen fuzz/fuzz_movegen.cpp ${SOURCES})
        target_compile_options(fuzz_movegen PRIVATE -fsanitize=fuzzer,address,undefined)
        target_link_options(fuzz_movegen PRIVATE -fsanitize=fuzzer,address,undefined)
        
        add_executable(fuzz_fen fuzz/fuzz_fen.cpp ${SOURCES})
        target_compile_options(fuzz_fen PRIVATE -fsanitize=fuzzer,address,undefined)
        target_link_options(fuzz_fen PRIVATE -fsanitize=fuzzer,address,undefined)
    else()
        message(WARNING "Fuzzing requires Clang compiler")
    endif()
endif()
```

## Best Practices

1. **Start with a good corpus** - Seed with valid inputs that exercise different code paths
2. **Use dictionaries** - Create a dictionary of chess-specific tokens (piece symbols, square names)
3. **Monitor coverage** - Use `-print_coverage=1` to track code coverage
4. **Run long enough** - Fuzzing needs hours or days to find deep bugs
5. **Minimize crashes** - Use `./fuzz_movegen -minimize_crash=1 crash-file` to simplify crash inputs
6. **Merge corpora** - Combine corpora from different runs: `./fuzz_movegen -merge=1 corpus_new corpus_old`

## Expected Findings

Fuzzing should help discover:

- Buffer overflows in move generation
- Integer overflows in evaluation
- Assertion failures in position validation
- Memory leaks
- Infinite loops or hangs
- Undefined behavior

## Resources

- [libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [AFL Documentation](https://github.com/google/AFL)
- [Fuzzing Best Practices](https://github.com/google/fuzzing)
