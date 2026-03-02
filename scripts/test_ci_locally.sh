#!/bin/bash
# Test CI pipeline locally before pushing
# This script runs the same checks that CI will run

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================================================"
echo "Local CI Test Runner"
echo -e "========================================================================${NC}"
echo ""

# Track failures
FAILURES=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_command=$2
    
    echo -e "${BLUE}Running: $test_name${NC}"
    
    if eval "$test_command"; then
        echo -e "${GREEN}✓ $test_name passed${NC}"
        echo ""
        return 0
    else
        echo -e "${RED}✗ $test_name failed${NC}"
        echo ""
        FAILURES=$((FAILURES + 1))
        return 1
    fi
}

# 1. Build Debug
run_test "Build (Debug)" "
    cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja 2>&1 | tail -5 &&
    cmake --build build-debug --parallel 2>&1 | tail -10
"

# 2. Build Release
run_test "Build (Release)" "
    cmake -B build-release -DCMAKE_BUILD_TYPE=Release -G Ninja 2>&1 | tail -5 &&
    cmake --build build-release --parallel 2>&1 | tail -10
"

# 3. Perft Tests
run_test "Perft Tests" "
    ./build-release/chess_engine <<EOF | grep -E 'perft [0-9]'
perft 1
perft 2
perft 3
perft 4
perft 5
quit
EOF
"

# 4. UCI Tests
run_test "UCI Protocol Tests" "
    echo 'uci' | ./build-release/chess_engine | grep -q 'uciok' &&
    echo 'isready' | ./build-release/chess_engine | grep -q 'readyok' &&
    ./build-release/chess_engine <<EOF | grep -q 'bestmove'
uci
isready
ucinewgame
position startpos
go depth 5
quit
EOF
"

# 5. Code Formatting Check
if command -v clang-format &> /dev/null; then
    run_test "Code Formatting" "
        find . \( -name '*.cpp' -o -name '*.h' \) \
            -not -path './build*' \
            -not -path './training/*' \
            -exec clang-format --dry-run --Werror {} \; 2>&1 | head -20
    " || echo -e "${YELLOW}Note: Some formatting issues found. Run 'find . -name \"*.cpp\" -o -name \"*.h\" | xargs clang-format -i' to fix${NC}"
else
    echo -e "${YELLOW}⚠ clang-format not found, skipping formatting check${NC}"
    echo ""
fi

# 6. Static Analysis
if command -v clang-tidy &> /dev/null; then
    run_test "Static Analysis (sample)" "
        cmake -B build-release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON &> /dev/null &&
        find . -name '*.cpp' -not -path './build*' -not -path './training/*' | head -3 | \
            xargs clang-tidy -p build-release --warnings-as-errors='' 2>&1 | tail -20
    " || echo -e "${YELLOW}Note: Some static analysis warnings found${NC}"
else
    echo -e "${YELLOW}⚠ clang-tidy not found, skipping static analysis${NC}"
    echo ""
fi

# 7. Benchmark
if [ -f "scripts/benchmark.py" ]; then
    run_test "Performance Benchmark" "
        python3 scripts/benchmark.py --engine ./build-release/chess_engine --output /tmp/benchmark_test.json 2>&1 | tail -15
    "
else
    echo -e "${YELLOW}⚠ Benchmark script not found, skipping benchmark${NC}"
    echo ""
fi

# Summary
echo -e "${BLUE}========================================================================"
echo "Summary"
echo -e "========================================================================${NC}"

if [ $FAILURES -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed! Ready to push.${NC}"
    exit 0
else
    echo -e "${RED}✗ $FAILURES test(s) failed. Please fix before pushing.${NC}"
    exit 1
fi
