# CI/CD Workflows Visual Guide

This document provides visual representations of the CI/CD workflows.

## CI Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│                         TRIGGER                                  │
│  • Push to main/develop                                         │
│  • Pull Request to main/develop                                 │
│  • Manual workflow dispatch                                     │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    BUILD & TEST (Matrix)                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│  │ Ubuntu GCC     │  │ Ubuntu Clang   │  │   macOS        │   │
│  │ Debug/Release  │  │ Debug/Release  │  │   Release      │   │
│  └────────────────┘  └────────────────┘  └────────────────┘   │
│                                                                  │
│  ┌────────────────┐                                             │
│  │ Windows MSVC   │                                             │
│  │ Debug/Release  │                                             │
│  └────────────────┘                                             │
│                                                                  │
│  Each configuration:                                            │
│  1. Cache restore (CMake + ccache)                             │
│  2. Install dependencies                                        │
│  3. Configure CMake                                             │
│  4. Build (parallel)                                            │
│  5. Run perft tests (depths 1-5)                               │
│  6. Run UCI protocol tests                                      │
│  7. Generate coverage (GCC Debug only)                         │
│  8. Upload artifacts                                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                        BENCHMARK                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Build Release (Ubuntu)                                      │
│  2. Run benchmark.py on 6 positions                            │
│  3. Download previous benchmark                                 │
│  4. Compare with compare_benchmarks.py                         │
│  5. Detect regressions (>5%)                                   │
│  6. Upload new benchmark results                                │
│  7. Comment on PR (if PR)                                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    STATIC ANALYSIS                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Install clang-tidy                                          │
│  2. Generate compile_commands.json                              │
│  3. Run clang-tidy on all .cpp files                           │
│  4. Report warnings                                             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    FORMAT CHECK                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Install clang-format                                        │
│  2. Check all .cpp and .h files                                │
│  3. Report formatting issues                                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                         SUMMARY                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ✓ All jobs passed → CI SUCCESS                                │
│  ✗ Any job failed → CI FAILURE                                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Release Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│                         TRIGGER                                  │
│  • Push tag (v*.*.*)                                            │
│  • Manual workflow dispatch                                     │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CREATE RELEASE                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Extract version from tag                                    │
│  2. Get previous tag                                            │
│  3. Generate release notes:                                     │
│     • Group commits by type (feat/fix/perf)                    │
│     • List all changes                                          │
│  4. Create GitHub release (draft=false)                        │
│  5. Output upload URL for binaries                              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
         ┌────────────────────┼────────────────────┐
         ↓                    ↓                    ↓
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  BUILD LINUX    │  │ BUILD WINDOWS   │  │  BUILD MACOS    │
├─────────────────┤  ├─────────────────┤  ├─────────────────┤
│                 │  │                 │  │                 │
│ 1. Checkout     │  │ 1. Checkout     │  │ 1. Checkout     │
│ 2. Install deps │  │ 2. Setup MSVC   │  │ 2. Install deps │
│ 3. Build Release│  │ 3. Build Release│  │ 3. Build x86_64 │
│ 4. Strip binary │  │ 4. Package .zip │  │ 4. Build ARM64  │
│ 5. Create .tar.gz│ │ 5. Upload       │  │ 5. Create lipo  │
│ 6. Upload       │  │                 │  │ 6. Create .tar.gz│
│                 │  │                 │  │ 7. Upload       │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │                    │                    │
         └────────────────────┼────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PUBLISH RELEASE                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ✓ All binaries uploaded                                        │
│  ✓ Release is public                                            │
│  ✓ Announcement complete                                        │
│                                                                  │
│  Artifacts:                                                      │
│  • chess_engine-linux-x86_64.tar.gz                            │
│  • chess_engine-windows-x86_64.zip                             │
│  • chess_engine-macos-universal.tar.gz                         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Benchmark Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    BENCHMARK POSITIONS                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. startpos     (depth 8)  → Starting position                │
│  2. kiwipete     (depth 7)  → Complex middlegame               │
│  3. endgame      (depth 10) → Complex endgame                  │
│  4. tactics      (depth 7)  → Tactical position                │
│  5. promotion    (depth 7)  → Promotion threats                │
│  6. middlegame   (depth 7)  → Balanced middlegame              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    FOR EACH POSITION                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Send UCI commands:                                          │
│     • uci                                                       │
│     • isready                                                   │
│     • ucinewgame                                                │
│     • position fen <FEN>                                        │
│     • go depth <DEPTH>                                          │
│     • quit                                                      │
│                                                                  │
│  2. Measure time                                                │
│  3. Parse output for nodes                                      │
│  4. Calculate NPS = nodes / time                                │
│  5. Store results                                               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    AGGREGATE RESULTS                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • Total nodes                                                  │
│  • Total time                                                   │
│  • Average NPS                                                  │
│  • Per-position NPS                                             │
│  • Export to JSON                                               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    COMPARISON (if previous exists)               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Load previous benchmark                                     │
│  2. Compare overall NPS                                         │
│  3. Compare per-position NPS                                    │
│  4. Calculate % change                                          │
│  5. Detect regressions (>5% slower)                            │
│  6. Detect improvements (>5% faster)                           │
│  7. Generate report                                             │
│                                                                  │
│  Output:                                                         │
│  • Table with old/new NPS                                       │
│  • Status (FASTER/SLOWER/STABLE)                               │
│  • Summary statistics                                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Perft Verification Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    PERFT TEST SUITE                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Depth 1:  Expected 20 nodes                                    │
│  Depth 2:  Expected 400 nodes                                   │
│  Depth 3:  Expected 8,902 nodes                                 │
│  Depth 4:  Expected 197,281 nodes                               │
│  Depth 5:  Expected 4,865,609 nodes                             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    FOR EACH DEPTH                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Send: perft <depth>                                         │
│  2. Parse output: perft <depth> <nodes>                         │
│  3. Compare with expected                                       │
│  4. Mark as PASS or FAIL                                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                         RESULT                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ✓ All depths match → Move generation CORRECT                  │
│  ✗ Any depth differs → Move generation BUG                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Local Testing Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    DEVELOPER WORKFLOW                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Make code changes                                           │
│  2. Run: ./scripts/test_ci_locally.sh                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    LOCAL CI CHECKS                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ✓ Build Debug                                                  │
│  ✓ Build Release                                                │
│  ✓ Perft Tests (depths 1-5)                                    │
│  ✓ UCI Protocol Tests                                           │
│  ✓ Code Formatting (clang-format)                              │
│  ✓ Static Analysis (clang-tidy)                                │
│  ✓ Performance Benchmark                                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
         ┌────────────────────┴────────────────────┐
         ↓                                         ↓
┌─────────────────┐                      ┌─────────────────┐
│  ALL PASS       │                      │  SOME FAIL      │
├─────────────────┤                      ├─────────────────┤
│                 │                      │                 │
│ ✓ Ready to push │                      │ ✗ Fix issues    │
│ ✓ CI will pass  │                      │ ✗ Run again     │
│                 │                      │                 │
└─────────────────┘                      └─────────────────┘
```

## Code Quality Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                    CODE SUBMISSION                               │
│  Developer pushes code or creates PR                            │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    FORMATTING CHECK                              │
│  clang-format --dry-run --Werror                                │
│  • Checks all .cpp and .h files                                 │
│  • Fails if formatting is incorrect                             │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    STATIC ANALYSIS                               │
│  clang-tidy -p build                                            │
│  • Checks for bugs                                              │
│  • Checks for anti-patterns                                     │
│  • Enforces modern C++                                          │
│  • Checks naming conventions                                    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    BUILD & TEST                                  │
│  • Compile with warnings as errors                              │
│  • Run all tests                                                │
│  • Verify correctness                                           │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CODE COVERAGE                                 │
│  • Measure test coverage                                        │
│  • Upload to Codecov                                            │
│  • Check against thresholds                                     │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PERFORMANCE CHECK                             │
│  • Run benchmarks                                               │
│  • Compare with baseline                                        │
│  • Detect regressions                                           │
└─────────────────────────────────────────────────────────────────┘
                              ↓
         ┌────────────────────┴────────────────────┐
         ↓                                         ↓
┌─────────────────┐                      ┌─────────────────┐
│  ALL PASS       │                      │  ANY FAIL       │
├─────────────────┤                      ├─────────────────┤
│                 │                      │                 │
│ ✓ Merge allowed │                      │ ✗ Fix required  │
│ ✓ High quality  │                      │ ✗ Cannot merge  │
│                 │                      │                 │
└─────────────────┘                      └─────────────────┘
```

## Caching Strategy

```
┌─────────────────────────────────────────────────────────────────┐
│                    CACHE KEY GENERATION                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Key: OS-Compiler-BuildType-CMakeLists.txt-Hash                │
│                                                                  │
│  Example:                                                        │
│  ubuntu-gcc-Release-a1b2c3d4e5f6...                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CACHE RESTORE                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Check for exact key match                                   │
│  2. If not found, check restore-keys:                           │
│     • OS-Compiler-BuildType-*                                   │
│     • OS-Compiler-*                                             │
│  3. Restore build/ and ~/.cache/ccache                         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    BUILD                                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • CMake uses cached build directory                            │
│  • ccache reuses compiled objects                               │
│  • Only changed files recompiled                                │
│  • Significant time savings (5-10x faster)                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CACHE SAVE                                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • Save build/ directory                                        │
│  • Save ~/.cache/ccache                                         │
│  • Use generated key                                            │
│  • Available for next run                                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Summary

This visual guide shows:

1. **CI Workflow**: Multi-stage testing and quality checks
2. **Release Workflow**: Automated binary builds and distribution
3. **Benchmark Flow**: Performance measurement and comparison
4. **Perft Verification**: Move generation correctness testing
5. **Local Testing**: Developer workflow before pushing
6. **Code Quality**: Comprehensive quality checks
7. **Caching Strategy**: Build acceleration techniques

All workflows are designed to:
- ✅ Catch issues early
- ✅ Maintain code quality
- ✅ Track performance
- ✅ Automate releases
- ✅ Support developers
- ✅ Provide fast feedback

For detailed documentation, see:
- **CI_CD_GUIDE.md**: Complete technical documentation
- **QUICK_CI_SETUP.md**: Quick start guide
- **CONTRIBUTING.md**: Contribution guidelines
