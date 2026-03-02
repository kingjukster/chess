#!/bin/bash
# Make all scripts executable

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Making scripts executable..."

chmod +x "$SCRIPT_DIR/benchmark.sh"
chmod +x "$SCRIPT_DIR/benchmark.py"
chmod +x "$SCRIPT_DIR/compare_benchmarks.py"
chmod +x "$SCRIPT_DIR/test_ci_locally.sh"
chmod +x "$SCRIPT_DIR/setup_scripts.sh"

echo "✓ All scripts are now executable"
echo ""
echo "Available scripts:"
echo "  - benchmark.sh              : Quick performance benchmark"
echo "  - benchmark.py              : Detailed benchmark with JSON output"
echo "  - compare_benchmarks.py     : Compare two benchmark results"
echo "  - test_ci_locally.sh        : Test CI pipeline locally"
echo ""
echo "Usage examples:"
echo "  ./scripts/benchmark.sh --engine ./build/chess_engine"
echo "  python3 scripts/benchmark.py --engine ./build/chess_engine --output results.json"
echo "  ./scripts/test_ci_locally.sh"
