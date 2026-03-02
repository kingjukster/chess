# CI/CD Pipeline Guide

This document describes the continuous integration and deployment pipelines for the Chess Engine project.

## Overview

The project uses GitHub Actions for automated testing, benchmarking, and releases. The CI/CD system ensures code quality, performance, and cross-platform compatibility.

## Workflows

### 1. CI Workflow (`.github/workflows/ci.yml`)

Runs on every push and pull request to `main` and `develop` branches.

#### Build Matrix

Tests across multiple configurations:

| Platform | Compiler | Build Types |
|----------|----------|-------------|
| Ubuntu   | GCC 11   | Debug, Release |
| Ubuntu   | Clang 14 | Debug, Release |
| macOS    | Clang    | Release |
| Windows  | MSVC     | Debug, Release |

#### Jobs

**1. build-and-test**
- Builds the engine for all platform/compiler combinations
- Runs perft tests (depths 1-5) to verify move generation
- Runs UCI protocol tests
- Generates code coverage (Ubuntu GCC Debug only)
- Uploads build artifacts

**2. benchmark**
- Runs performance benchmarks on standard positions
- Compares against previous benchmark results
- Detects performance regressions (>5% slowdown)
- Comments results on pull requests
- Uploads benchmark data for historical tracking

**3. static-analysis**
- Runs clang-tidy for code quality checks
- Checks for common bugs and anti-patterns
- Enforces modern C++ best practices

**4. format-check**
- Verifies code formatting with clang-format
- Ensures consistent code style

**5. summary**
- Aggregates results from all jobs
- Fails if any critical job fails

#### Caching

The CI uses caching to speed up builds:
- CMake build directories
- ccache compilation cache
- Keyed by OS, compiler, build type, and CMakeLists.txt hash

### 2. Release Workflow (`.github/workflows/release.yml`)

Triggered by:
- Pushing a version tag (e.g., `v1.0.0`)
- Manual workflow dispatch

#### Jobs

**1. create-release**
- Generates release notes from git history
- Groups commits by type (features, fixes, performance)
- Creates GitHub release

**2. build-linux**
- Builds optimized Linux binary
- Strips debug symbols
- Creates `.tar.gz` package
- Uploads to GitHub release

**3. build-windows**
- Builds optimized Windows binary
- Creates `.zip` package
- Uploads to GitHub release

**4. build-macos**
- Builds universal binary (x86_64 + ARM64)
- Creates `.tar.gz` package
- Uploads to GitHub release

**5. publish-release**
- Finalizes release
- Announces completion

## Setting Up CI/CD

### Required Secrets

Add these secrets to your GitHub repository (Settings → Secrets and variables → Actions):

1. **CODECOV_TOKEN** (optional)
   - For code coverage reporting
   - Get from https://codecov.io after adding your repository

### Badge Setup

Update the badges in `README.md` with your repository information:

```markdown
[![CI](https://github.com/USERNAME/REPO/workflows/CI/badge.svg)](...)
[![Release](https://github.com/USERNAME/REPO/workflows/Release/badge.svg)](...)
[![codecov](https://codecov.io/gh/USERNAME/REPO/branch/main/graph/badge.svg)](...)
```

Replace `USERNAME` and `REPO` with your GitHub username and repository name.

## Benchmarking

### Standard Benchmark Positions

The benchmark suite includes 6 positions:

1. **startpos**: Starting position (depth 8)
2. **kiwipete**: Complex middlegame (depth 7)
3. **endgame**: Complex endgame (depth 10)
4. **tactics**: Tactical position (depth 7)
5. **promotion**: Promotion threats (depth 7)
6. **middlegame**: Balanced middlegame (depth 7)

### Running Benchmarks Locally

```bash
# Full benchmark with JSON output
python3 scripts/benchmark.py --engine ./build/chess_engine --output results.json

# Quick benchmark
./scripts/benchmark.sh --engine ./build/chess_engine

# With perft tests
python3 scripts/benchmark.py --engine ./build/chess_engine --perft
```

### Comparing Benchmarks

```bash
# Compare two benchmark runs
python3 scripts/compare_benchmarks.py old_results.json new_results.json

# Fail if regression detected
python3 scripts/compare_benchmarks.py old.json new.json --fail-on-regression
```

### Performance Regression Detection

The CI automatically detects performance regressions:
- Threshold: 5% slowdown
- Compares against previous commit's benchmark
- Comments on pull requests with results
- Fails if significant regression detected (optional)

## Code Quality

### Formatting

Code formatting is enforced with clang-format:

```bash
# Format all files
find . -name '*.cpp' -o -name '*.h' | \
  grep -v './build/' | \
  xargs clang-format -i

# Check formatting (CI mode)
clang-format --dry-run --Werror <file>
```

Configuration: `.clang-format`

### Linting

Static analysis with clang-tidy:

```bash
# Generate compile commands
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-tidy
clang-tidy -p build <file.cpp>

# Run on all files
find . -name '*.cpp' | xargs clang-tidy -p build
```

Configuration: `.clang-tidy`

### Code Coverage

Coverage is collected on Ubuntu GCC Debug builds:

```bash
# Build with coverage
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build

# Run tests
./build/chess_engine

# Generate report
lcov --directory build --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage
```

Configuration: `.codecov.yml`

## Release Process

### Creating a Release

1. **Tag the release**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. **Automatic build**
   - GitHub Actions builds binaries for all platforms
   - Creates GitHub release with binaries
   - Generates release notes from commits

3. **Manual release** (optional)
   - Go to Actions → Release workflow
   - Click "Run workflow"
   - Enter version tag (e.g., `v1.0.0`)

### Release Artifacts

Each release includes:
- `chess_engine-linux-x86_64.tar.gz`
- `chess_engine-windows-x86_64.zip`
- `chess_engine-macos-universal.tar.gz`
- Automatically generated release notes

## Troubleshooting

### CI Failures

**Build fails on specific platform:**
- Check compiler-specific errors in logs
- Test locally with same compiler version
- Check CMake configuration

**Perft tests fail:**
- Move generation bug introduced
- Run perft locally to debug
- Compare with known-good results

**Benchmark regression:**
- Check if change was intentional
- Profile to find performance bottleneck
- Consider optimization or revert

**Coverage upload fails:**
- Check CODECOV_TOKEN secret
- Verify coverage.info is generated
- Check codecov.io service status

### Local Testing

Test CI locally before pushing:

```bash
# Build all configurations
for config in Debug Release; do
  cmake -B build-$config -DCMAKE_BUILD_TYPE=$config
  cmake --build build-$config
done

# Run tests
./build-Release/chess_engine <<EOF
perft 1
perft 2
perft 3
perft 4
perft 5
quit
EOF

# Run benchmarks
python3 scripts/benchmark.py --engine ./build-Release/chess_engine

# Check formatting
find . -name '*.cpp' -o -name '*.h' | \
  grep -v './build' | \
  xargs clang-format --dry-run --Werror

# Run static analysis
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
find . -name '*.cpp' | grep -v './build' | xargs clang-tidy -p build
```

## Best Practices

1. **Always run tests locally before pushing**
2. **Keep commits focused and atomic**
3. **Write descriptive commit messages**
4. **Monitor CI results and fix failures promptly**
5. **Review benchmark results for performance changes**
6. **Update documentation when adding features**
7. **Tag releases with semantic versioning (vX.Y.Z)**

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [CMake Documentation](https://cmake.org/documentation/)
- [Codecov Documentation](https://docs.codecov.io/)
- [Clang-Format Documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
