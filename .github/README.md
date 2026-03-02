# GitHub Actions CI/CD

This directory contains the GitHub Actions workflows and documentation for the Chess Engine project's CI/CD pipeline.

## 📁 Directory Structure

```
.github/
├── workflows/
│   ├── ci.yml              # Continuous Integration workflow
│   └── release.yml         # Release automation workflow
├── CI_CD_GUIDE.md          # Comprehensive CI/CD documentation
├── WORKFLOWS.md            # Visual workflow diagrams
└── README.md               # This file
```

## 🚀 Workflows

### CI Workflow (`workflows/ci.yml`)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual workflow dispatch

**Jobs:**
1. **build-and-test**: Multi-platform builds with 7 configurations
   - Ubuntu (GCC Debug/Release, Clang Debug/Release)
   - Windows (MSVC Debug/Release)
   - macOS (Clang Release)

2. **benchmark**: Performance testing
   - Runs on 6 standard positions
   - Compares with previous results
   - Detects regressions (>5%)
   - Comments on PRs

3. **static-analysis**: Code quality checks
   - clang-tidy static analysis
   - Modern C++ best practices

4. **format-check**: Code formatting
   - clang-format verification
   - Consistent code style

5. **summary**: Aggregates all results

**Duration:** ~15-20 minutes (with caching)

### Release Workflow (`workflows/release.yml`)

**Triggers:**
- Push tags matching `v*.*.*` (e.g., `v1.0.0`)
- Manual workflow dispatch

**Jobs:**
1. **create-release**: Generate release notes and create GitHub release
2. **build-linux**: Build and package Linux binary (.tar.gz)
3. **build-windows**: Build and package Windows binary (.zip)
4. **build-macos**: Build universal binary for macOS (.tar.gz)
5. **publish-release**: Finalize and announce release

**Artifacts:**
- `chess_engine-linux-x86_64.tar.gz`
- `chess_engine-windows-x86_64.zip`
- `chess_engine-macos-universal.tar.gz`

**Duration:** ~10-15 minutes

## 📊 Build Matrix

| OS | Compiler | Versions | Build Types |
|----|----------|----------|-------------|
| Ubuntu 22.04 | GCC | 11 | Debug, Release |
| Ubuntu 22.04 | Clang | 14 | Debug, Release |
| macOS 13 | AppleClang | Latest | Release |
| Windows 2022 | MSVC | 2022 | Debug, Release |

## 🎯 Quality Gates

All PRs must pass:
- ✅ Build on all platforms
- ✅ Perft tests (depths 1-5)
- ✅ UCI protocol tests
- ✅ Performance benchmarks
- ✅ Static analysis
- ✅ Code formatting

## 🔧 Configuration

### Secrets Required

- `CODECOV_TOKEN` (optional): For code coverage reporting
  - Get from https://codecov.io
  - Add in: Settings → Secrets and variables → Actions

### Badges

Update in `README.md`:

```markdown
[![CI](https://github.com/USERNAME/REPO/workflows/CI/badge.svg)](https://github.com/USERNAME/REPO/actions/workflows/ci.yml)
[![Release](https://github.com/USERNAME/REPO/workflows/Release/badge.svg)](https://github.com/USERNAME/REPO/actions/workflows/release.yml)
[![codecov](https://codecov.io/gh/USERNAME/REPO/branch/main/graph/badge.svg)](https://codecov.io/gh/USERNAME/REPO)
```

## 📈 Performance Benchmarking

### Benchmark Positions

1. **startpos** (depth 8): Starting position
2. **kiwipete** (depth 7): Complex middlegame
3. **endgame** (depth 10): Complex endgame
4. **tactics** (depth 7): Tactical position
5. **promotion** (depth 7): Promotion threats
6. **middlegame** (depth 7): Balanced middlegame

### Regression Detection

- Threshold: 5% slowdown
- Compares: Current vs. previous commit
- Action: Comments on PR if regression detected

## 🛠️ Local Testing

Test CI locally before pushing:

```bash
# Make scripts executable
chmod +x scripts/*.sh scripts/*.py

# Verify setup
./scripts/verify_ci_setup.sh

# Run all CI checks
./scripts/test_ci_locally.sh
```

## 📚 Documentation

- **[CI_CD_GUIDE.md](CI_CD_GUIDE.md)**: Complete technical documentation
  - Workflow details
  - Setup instructions
  - Troubleshooting
  - Best practices

- **[WORKFLOWS.md](WORKFLOWS.md)**: Visual workflow diagrams
  - CI flow
  - Release flow
  - Benchmark flow
  - Testing flow

- **[../QUICK_CI_SETUP.md](../QUICK_CI_SETUP.md)**: 5-minute quick start
  - Step-by-step setup
  - Badge configuration
  - Verification steps

- **[../CONTRIBUTING.md](../CONTRIBUTING.md)**: Contribution guidelines
  - Development setup
  - Code style
  - Testing requirements
  - PR process

## 🔍 Monitoring

### Check Workflow Status

1. Go to repository on GitHub
2. Click "Actions" tab
3. View workflow runs and logs

### View Benchmark History

Benchmark results are stored as artifacts for 90 days:
1. Go to Actions → CI workflow
2. Select a run
3. Download "benchmark-results" artifact

### Coverage Reports

View on Codecov (if configured):
- https://codecov.io/gh/USERNAME/REPO

## 🚨 Troubleshooting

### CI Fails on Push

1. Check workflow logs in Actions tab
2. Identify failing job
3. Review error messages
4. Test locally: `./scripts/test_ci_locally.sh`
5. Fix issues and push again

### Perft Tests Fail

```bash
# Test locally
echo "perft 5" | ./build/chess_engine

# Expected: 4865609 nodes
# If different, debug move generation
```

### Benchmark Regression

```bash
# Run benchmark locally
python3 scripts/benchmark.py --engine ./build/chess_engine --output baseline.json

# After changes
python3 scripts/benchmark.py --engine ./build/chess_engine --output new.json

# Compare
python3 scripts/compare_benchmarks.py baseline.json new.json
```

### Release Workflow Doesn't Trigger

- Verify tag format: `v1.0.0` (must start with 'v')
- Check tag is pushed: `git push origin v1.0.0`
- Ensure release.yml exists in `.github/workflows/`

## 🎓 Learning Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
- [CMake Documentation](https://cmake.org/documentation/)
- [Codecov Documentation](https://docs.codecov.io/)

## 📝 Customization

### Add New Build Configuration

Edit `workflows/ci.yml`:

```yaml
- {
    name: "Ubuntu GCC 12",
    os: ubuntu-latest,
    compiler: gcc,
    cc: "gcc-12",
    cxx: "g++-12",
    build_type: "Release",
    coverage: false
  }
```

### Adjust Benchmark Depth

Edit `scripts/benchmark.py`:

```python
BENCHMARK_POSITIONS = {
    "startpos": {
        "depth": 10,  # Increase depth
        ...
    }
}
```

### Change Regression Threshold

Edit `workflows/ci.yml`:

```yaml
python3 scripts/compare_benchmarks.py \
  --threshold 0.10  # 10% instead of 5%
```

## 🤝 Contributing

When modifying workflows:

1. Test locally first
2. Use workflow validation: `act` or GitHub's workflow validator
3. Document changes in this README
4. Update CI_CD_GUIDE.md if needed
5. Test on a fork before merging

## 📊 Statistics

- **Total Lines**: ~3,000+ (workflows + scripts + docs)
- **Workflows**: 2 (CI + Release)
- **Build Configurations**: 7
- **Platforms**: 3 (Linux, Windows, macOS)
- **Benchmark Positions**: 6
- **Perft Depths**: 5
- **Documentation**: 2,000+ lines

## ✅ Success Criteria

Your CI/CD is working when:

- ✅ Green checkmarks on all commits
- ✅ Badges show "passing" in README
- ✅ Benchmarks complete without regression
- ✅ Releases create binaries automatically
- ✅ Coverage reports upload (if configured)
- ✅ Contributors can test locally

## 📞 Support

- **Issues**: Open a GitHub issue
- **Discussions**: Use GitHub Discussions
- **Documentation**: See CI_CD_GUIDE.md
- **Quick Help**: See QUICK_CI_SETUP.md

---

**Last Updated**: March 1, 2026  
**Maintainer**: Chess Engine Contributors  
**License**: MIT
