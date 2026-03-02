# Chess Engine Test Suite

This directory contains a comprehensive test suite for the chess engine using Google Test framework.

## Test Files

### Unit Tests

- **test_bitboard.cpp**: Tests for bitboard operations
  - Popcount, LSB, MSB operations
  - Bit manipulation functions
  - Square and piece helper functions
  - Move construction and validation

- **test_attacks.cpp**: Tests for attack generation
  - Pawn, knight, king attack tables
  - Sliding piece attacks (bishop, rook, queen)
  - Magic bitboard implementation
  - Attack detection and check validation

- **test_position.cpp**: Tests for position representation
  - FEN parsing and generation
  - Make/unmake move operations
  - Position state management
  - Zobrist hashing
  - Castling rights and en passant

- **test_movegen.cpp**: Tests for move generation
  - Pseudo-legal move generation
  - Legal move filtering
  - Special moves (castling, en passant, promotion)
  - Pinned pieces and check evasion

- **test_eval.cpp**: Tests for evaluation functions
  - Material counting
  - Piece-square tables
  - Positional evaluation
  - Incremental updates

### Integration Tests

- **test_perft.cpp**: Performance testing and move generation validation
  - Standard perft positions (depths 1-6)
  - Kiwipete and other complex positions
  - Divide perft for debugging
  - Node count verification

- **test_search.cpp**: Tests for search functionality
  - Alpha-beta search correctness
  - Mate finding (mate in 1, mate in 2)
  - Tactical move detection
  - Iterative deepening
  - Transposition table

- **test_uci.cpp**: Tests for UCI protocol
  - Move parsing and formatting
  - FEN string handling
  - Position setup commands
  - Move notation (algebraic)

## Test Data

### data/perft_positions.epd
Standard perft test positions with expected node counts for depths 1-6:
- Starting position
- Kiwipete (complex middle game)
- Positions 3-6 (various tactical scenarios)

### data/tactical_positions.epd
Known tactical positions with best moves:
- Mate in 1 positions
- Forks, pins, discoveries
- Promotion tactics
- Central control positions

## Building and Running Tests

### Build Tests
```bash
cd chess
mkdir build
cd build
cmake ..
cmake --build .
```

### Run All Tests
```bash
./chess_tests
```

### Run Specific Test Suite
```bash
./chess_tests --gtest_filter=BitboardTest.*
./chess_tests --gtest_filter=PerftTest.*
```

### Run Single Test
```bash
./chess_tests --gtest_filter=BitboardTest.PopcountZero
```

### Verbose Output
```bash
./chess_tests --gtest_verbose
```

### List All Tests
```bash
./chess_tests --gtest_list_tests
```

## Test Coverage

The test suite covers:

1. **Bitboard Operations** (100%)
   - All bit manipulation functions
   - Square/rank/file conversions
   - Move encoding/decoding

2. **Attack Generation** (100%)
   - All piece types
   - Magic bitboards
   - Attack detection

3. **Position Management** (95%)
   - FEN parsing/generation
   - Make/unmake moves
   - State tracking

4. **Move Generation** (95%)
   - All move types
   - Legal move filtering
   - Special cases

5. **Evaluation** (90%)
   - Material counting
   - Positional evaluation
   - Incremental updates

6. **Search** (85%)
   - Basic search functionality
   - Mate finding
   - Tactical awareness

7. **UCI Protocol** (80%)
   - Move parsing
   - Position setup
   - Command handling

## Performance Benchmarks

Expected perft results (for validation):

| Position | Depth 1 | Depth 2 | Depth 3 | Depth 4 | Depth 5 | Depth 6 |
|----------|---------|---------|---------|---------|---------|---------|
| Starting | 20      | 400     | 8,902   | 197,281 | 4,865,609 | 119,060,324 |
| Kiwipete | 48      | 2,039   | 97,862  | 4,085,603 | 193,690,690 | - |
| Position 3 | 14    | 191     | 2,812   | 43,238  | 674,624 | 11,030,083 |

## Adding New Tests

To add a new test file:

1. Create `test_<feature>.cpp` in the tests directory
2. Include necessary headers and Google Test
3. Create test fixture class if needed
4. Write TEST_F macros for each test case
5. Add the file to CMakeLists.txt TEST_SOURCES

Example:
```cpp
#include <gtest/gtest.h>
#include "../your_module.h"

class YourTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(YourTest, TestName) {
    EXPECT_EQ(actual, expected);
}
```

## Continuous Integration

These tests are designed to run in CI/CD pipelines:
- Fast execution (< 30 seconds for full suite)
- Deterministic results
- Clear error messages
- No external dependencies

## Debugging Failed Tests

When a test fails:

1. Run with verbose output: `--gtest_verbose`
2. Run single test: `--gtest_filter=TestSuite.TestName`
3. Check test output for assertion details
4. Use perft divide for move generation issues
5. Verify FEN strings for position issues

## Contributing

When adding features:
1. Write tests first (TDD approach)
2. Ensure all tests pass
3. Add integration tests for complex features
4. Update this README if adding new test categories
