#!/bin/bash
# Build fuzzing harnesses

set -e

echo "Building fuzzing harnesses..."

# Check for clang
if ! command -v clang++ &> /dev/null; then
    echo "Error: clang++ not found. Please install Clang."
    exit 1
fi

# Build directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Source files
BOARD_SRC="../board/position.cpp"
MOVEGEN_SRC="../movegen/movegen.cpp ../movegen/attacks.cpp"

# Compiler flags
FUZZ_FLAGS="-fsanitize=fuzzer,address,undefined -g -O1 -std=c++17 -I.."

echo "Building fuzz_movegen..."
clang++ $FUZZ_FLAGS \
    fuzz_movegen.cpp \
    $BOARD_SRC \
    $MOVEGEN_SRC \
    -o fuzz_movegen

echo "Building fuzz_fen..."
clang++ $FUZZ_FLAGS \
    fuzz_fen.cpp \
    $BOARD_SRC \
    ../movegen/attacks.cpp \
    -o fuzz_fen

echo "Fuzzers built successfully!"
echo ""
echo "To run:"
echo "  mkdir -p corpus_movegen corpus_fen"
echo "  ./fuzz_movegen corpus_movegen"
echo "  ./fuzz_fen corpus_fen"
