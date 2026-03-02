# Testing Framework Quick Start Guide

## Building and Running Tests

### First Time Setup

```bash
cd c:\Users\horne\projects\chess_engine\chess
mkdir build
cd build
cmake .. -DBUILD_TESTS=ON
```

### Build Tests

```bash
# From chess/build directory
cmake --build . --target chess_tests --config Release
```

### Run All Tests

```bash
# From chess/build directory
.\Release\chess_tests.exe
```

## Common Test Commands

### Run Specific Test Suite

```bash
# Run only bitboard tests
.\Release\chess_tests.exe --gtest_filter=BitboardTest.*

# Run only perft tests
.\Release\chess_tests.exe --gtest_filter=PerftTest.*

# Run only search tests
.\Release\chess_tests.exe --gtest_filter=SearchTest.*
```

### Run Single Test

```bash
# Run a specific test
.\Release\chess_tests.exe --gtest_filter=BitboardTest.PopcountZero

# Run mate-finding tests
.\Release\chess_tests.exe --gtest_filter=SearchTest.FindsMateInOne
```

### Useful Flags

```bash
# Brief output (less verbose)
.\Release\chess_tests.exe --gtest_brief=1

# List all tests without running
.\Release\chess_tests.exe --gtest_list_tests

# Repeat tests (useful for finding flaky tests)
.\Release\chess_tests.exe --gtest_repeat=10

# Run tests in random order
.\Release\chess_tests.exe --gtest_shuffle

# Stop on first failure
.\Release\chess_tests.exe --gtest_break_on_failure
```

## Test Categories

### Unit Tests (Fast)
```bash
# Bitboard operations
.\Release\chess_tests.exe --gtest_filter=BitboardTest.*

# Attack generation
.\Release\chess_tests.exe --gtest_filter=AttacksTest.*

# Position management
.\Release\chess_tests.exe --gtest_filter=PositionTest.*

# Evaluation
.\Release\chess_tests.exe --gtest_filter=EvalTest.*
```

### Integration Tests (Medium)
```bash
# Move generation
.\Release\chess_tests.exe --gtest_filter=MoveGenTest.*

# UCI protocol
.\Release\chess_tests.exe --gtest_filter=UCITest.*
```

### Performance Tests (Slow)
```bash
# Perft tests (can be slow at high depths)
.\Release\chess_tests.exe --gtest_filter=PerftTest.*

# Search tests
.\Release\chess_tests.exe --gtest_filter=SearchTest.*
```

## Debugging Failed Tests

### 1. Identify the Failure

```bash
# Run with verbose output
.\Release\chess_tests.exe --gtest_filter=FailingTest.* --gtest_verbose
```

### 2. Run Single Test

```bash
# Isolate the specific failing test
.\Release\chess_tests.exe --gtest_filter=PerftTest.StartingPositionDepth1
```

### 3. Check Test Output

The test will show:
- Expected value
- Actual value
- Line number in test file
- Assertion that failed

Example:
```
test_perft.cpp(39): error: Expected equality of these values:
  nodes
    Which is: 38
  20
Starting position depth 1
```

### 4. Fix the Bug

Based on the test output:
1. Locate the source file
2. Find the function being tested
3. Add debug output if needed
4. Fix the bug
5. Rebuild and rerun test

## Adding New Tests

### 1. Create Test Function

```cpp
TEST_F(YourTestFixture, TestName) {
    // Arrange
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    // Act
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    // Assert
    EXPECT_EQ(moves.size(), 20);
}
```

### 2. Add to CMakeLists.txt

If creating a new test file:
```cmake
set(TEST_SOURCES
    tests/test_bitboard.cpp
    tests/test_movegen.cpp
    tests/your_new_test.cpp  # Add here
    # ... other tests
)
```

### 3. Rebuild

```bash
cmake --build . --target chess_tests --config Release
```

## Test Data Files

### Perft Positions

Located at: `tests/data/perft_positions.epd`

Format:
```
FEN_STRING ;D1 nodes ;D2 nodes ;D3 nodes ...
```

Example:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ;D1 20 ;D2 400 ;D3 8902
```

### Tactical Positions

Located at: `tests/data/tactical_positions.epd`

Format:
```
FEN_STRING bm BEST_MOVE; id "Description";
```

Example:
```
k7/8/1K6/8/8/8/8/7R w - - 0 1 bm Rh8#; id "Mate in 1 - Back rank";
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Configure
        run: cmake -B build -DBUILD_TESTS=ON
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: cd build && .\Release\chess_tests.exe
```

## Performance Benchmarks

### Expected Perft Times (Release build)

| Position | Depth | Nodes | Time (approx) |
|----------|-------|-------|---------------|
| Starting | 1     | 20    | < 1ms         |
| Starting | 2     | 400   | < 1ms         |
| Starting | 3     | 8,902 | < 10ms        |
| Starting | 4     | 197,281 | < 100ms     |
| Starting | 5     | 4,865,609 | < 2s        |
| Kiwipete | 4     | 4,085,603 | < 2s        |

### Running Performance Tests

```bash
# Time the perft tests
Measure-Command { .\Release\chess_tests.exe --gtest_filter=PerftTest.* }
```

## Troubleshooting

### Tests Won't Build

1. Check CMake configuration:
```bash
cmake .. -DBUILD_TESTS=ON
```

2. Clean build directory:
```bash
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -DBUILD_TESTS=ON
```

### Tests Crash

1. Run in debugger (Visual Studio)
2. Check for null pointers
3. Verify attack tables are initialized
4. Check array bounds

### Tests Are Slow

1. Build in Release mode:
```bash
cmake --build . --config Release
```

2. Run only fast tests:
```bash
.\Release\chess_tests.exe --gtest_filter=-PerftTest.*:-SearchTest.*
```

### Google Test Not Found

CMake automatically downloads Google Test via FetchContent. If it fails:

1. Check internet connection
2. Clear CMake cache:
```bash
Remove-Item CMakeCache.txt
cmake .. -DBUILD_TESTS=ON
```

## Best Practices

### 1. Run Tests Before Committing

```bash
.\Release\chess_tests.exe
```

### 2. Write Tests for New Features

Before implementing a feature:
1. Write the test
2. Verify it fails
3. Implement the feature
4. Verify test passes

### 3. Keep Tests Fast

- Unit tests should run in milliseconds
- Integration tests in seconds
- Performance tests can take longer

### 4. Use Descriptive Names

Good:
```cpp
TEST_F(MoveGenTest, StartingPositionGenerates20Moves)
```

Bad:
```cpp
TEST_F(MoveGenTest, Test1)
```

### 5. One Assertion Per Test (when possible)

This makes failures easier to diagnose.

## Getting Help

### Documentation
- `tests/README.md` - Comprehensive guide
- `tests/TEST_RESULTS.md` - Current test status
- `TESTING_FRAMEWORK_SUMMARY.md` - Implementation details

### Google Test Documentation
- https://google.github.io/googletest/

### Common Issues
1. **Move encoding bugs** - Check `types.h` Move struct
2. **Attack generation** - Verify magic bitboards initialized
3. **Perft failures** - Usually move generation bugs
4. **SEH exceptions** - Null pointers or array bounds

## Quick Reference

```bash
# Build tests
cmake --build . --target chess_tests --config Release

# Run all tests
.\Release\chess_tests.exe

# Run specific suite
.\Release\chess_tests.exe --gtest_filter=BitboardTest.*

# Run single test
.\Release\chess_tests.exe --gtest_filter=BitboardTest.PopcountZero

# List tests
.\Release\chess_tests.exe --gtest_list_tests

# Brief output
.\Release\chess_tests.exe --gtest_brief=1

# Stop on first failure
.\Release\chess_tests.exe --gtest_break_on_failure
```
