# CI/CD Pipeline - Complete Index

This is your one-stop reference for all CI/CD documentation and resources.

## 🚀 Quick Start

**New to the project?** Start here:

1. **[QUICK_CI_SETUP.md](QUICK_CI_SETUP.md)** - 5-minute setup guide
   - Step-by-step instructions
   - Badge configuration
   - First release creation

2. **[scripts/verify_ci_setup.sh](scripts/verify_ci_setup.sh)** - Verify your setup
   ```bash
   chmod +x scripts/verify_ci_setup.sh
   ./scripts/verify_ci_setup.sh
   ```

3. **[scripts/test_ci_locally.sh](scripts/test_ci_locally.sh)** - Test before pushing
   ```bash
   ./scripts/test_ci_locally.sh
   ```

## 📚 Documentation

### Core Documentation

| Document | Description | Lines | Audience |
|----------|-------------|-------|----------|
| **[.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md)** | Complete technical guide | 400+ | Developers, Maintainers |
| **[.github/WORKFLOWS.md](.github/WORKFLOWS.md)** | Visual workflow diagrams | 500+ | All users |
| **[QUICK_CI_SETUP.md](QUICK_CI_SETUP.md)** | Quick start guide | 300+ | New users |
| **[CI_CD_SUMMARY.md](CI_CD_SUMMARY.md)** | Implementation summary | 600+ | Project managers |
| **[CONTRIBUTING.md](CONTRIBUTING.md)** | Contribution guidelines | 200+ | Contributors |

### Specialized Documentation

| Document | Description | Audience |
|----------|-------------|----------|
| **[.github/README.md](.github/README.md)** | GitHub Actions overview | Maintainers |
| **[scripts/README.md](scripts/README.md)** | Scripts documentation | Developers |
| **[README.md](README.md)** | Main project README | All users |

## 🔧 Workflows

### GitHub Actions Workflows

| Workflow | File | Triggers | Duration |
|----------|------|----------|----------|
| **CI** | [.github/workflows/ci.yml](.github/workflows/ci.yml) | Push, PR | ~15-20 min |
| **Release** | [.github/workflows/release.yml](.github/workflows/release.yml) | Tags (v*.*.*) | ~10-15 min |

### Workflow Features

**CI Workflow:**
- ✅ 7 build configurations (Ubuntu, Windows, macOS)
- ✅ Perft tests (depths 1-5)
- ✅ UCI protocol tests
- ✅ Performance benchmarks
- ✅ Code coverage
- ✅ Static analysis
- ✅ Format checking

**Release Workflow:**
- ✅ Automated binary builds
- ✅ Multi-platform releases
- ✅ Release notes generation
- ✅ GitHub release creation

## 🛠️ Scripts

### Benchmarking

| Script | Language | Platform | Description |
|--------|----------|----------|-------------|
| **[scripts/benchmark.py](scripts/benchmark.py)** | Python | All | Comprehensive benchmark with JSON output |
| **[scripts/benchmark.sh](scripts/benchmark.sh)** | Bash | Unix/Linux/macOS | Quick shell-based benchmark |
| **[scripts/benchmark.bat](scripts/benchmark.bat)** | Batch | Windows | Windows-native benchmark |
| **[scripts/compare_benchmarks.py](scripts/compare_benchmarks.py)** | Python | All | Compare benchmark results |

### Testing

| Script | Description | Usage |
|--------|-------------|-------|
| **[scripts/test_ci_locally.sh](scripts/test_ci_locally.sh)** | Run all CI checks locally | `./scripts/test_ci_locally.sh` |
| **[scripts/verify_ci_setup.sh](scripts/verify_ci_setup.sh)** | Verify CI/CD setup | `./scripts/verify_ci_setup.sh` |
| **[scripts/setup_scripts.sh](scripts/setup_scripts.sh)** | Make scripts executable | `./scripts/setup_scripts.sh` |

## ⚙️ Configuration Files

| File | Purpose | Documentation |
|------|---------|---------------|
| **[.clang-format](.clang-format)** | Code formatting rules | [Clang-Format Docs](https://clang.llvm.org/docs/ClangFormat.html) |
| **[.clang-tidy](.clang-tidy)** | Static analysis rules | [Clang-Tidy Docs](https://clang.llvm.org/extra/clang-tidy/) |
| **[.codecov.yml](.codecov.yml)** | Code coverage config | [Codecov Docs](https://docs.codecov.io/) |
| **[LICENSE](LICENSE)** | MIT License | [MIT License](https://opensource.org/licenses/MIT) |

## 📊 Benchmark Positions

Standard positions used for performance testing:

| Position | Depth | Description | Purpose |
|----------|-------|-------------|---------|
| startpos | 8 | Starting position | Basic performance |
| kiwipete | 7 | Complex middlegame | Castling, en passant |
| endgame | 10 | Complex endgame | Deep search |
| tactics | 7 | Tactical position | Quiescence search |
| promotion | 7 | Promotion threats | Promotion handling |
| middlegame | 7 | Balanced middlegame | Typical evaluation |

## 🧪 Testing

### Perft Tests

Verify move generation correctness:

| Depth | Expected Nodes | Purpose |
|-------|----------------|---------|
| 1 | 20 | Basic moves |
| 2 | 400 | Two-ply |
| 3 | 8,902 | Three-ply |
| 4 | 197,281 | Four-ply |
| 5 | 4,865,609 | Five-ply |

### UCI Tests

- `uci` → `uciok`
- `isready` → `readyok`
- `position startpos` + `go depth 5` → `bestmove`

## 🎯 Common Tasks

### For Developers

**Before pushing:**
```bash
./scripts/test_ci_locally.sh
```

**Run benchmarks:**
```bash
python3 scripts/benchmark.py --engine ./build/chess_engine --output results.json
```

**Compare performance:**
```bash
python3 scripts/compare_benchmarks.py old.json new.json
```

**Format code:**
```bash
find . -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### For Maintainers

**Create release:**
```bash
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

**Check CI status:**
- Go to GitHub → Actions tab
- View workflow runs

**Update badges:**
- Edit README.md
- Replace `YOUR_USERNAME/YOUR_REPO`

### For Contributors

**Setup:**
```bash
git clone https://github.com/USERNAME/REPO.git
cd REPO
./scripts/setup_scripts.sh
./scripts/verify_ci_setup.sh
```

**Test changes:**
```bash
./scripts/test_ci_locally.sh
```

**Submit PR:**
1. Create feature branch
2. Make changes
3. Test locally
4. Push and create PR
5. Wait for CI to pass

## 🔍 Troubleshooting

### Quick Fixes

| Problem | Solution | Reference |
|---------|----------|-----------|
| CI fails | Check Actions logs | [.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md#troubleshooting) |
| Perft wrong | Debug move generation | [.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md#perft-tests-fail) |
| Benchmark slow | Check depth settings | [scripts/README.md](scripts/README.md#troubleshooting) |
| Scripts not executable | Run setup_scripts.sh | [scripts/README.md](scripts/README.md#permission-denied-error) |
| Badge not showing | Update README | [QUICK_CI_SETUP.md](QUICK_CI_SETUP.md#step-1-update-repository-information) |

### Detailed Troubleshooting

See [.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md#troubleshooting) for comprehensive troubleshooting guide.

## 📈 Performance Tracking

### Metrics Tracked

- **Nodes per second (NPS)**: Primary performance metric
- **Total nodes**: Search tree size
- **Time per position**: Search duration
- **Perft speed**: Move generation speed
- **Code coverage**: Test coverage percentage

### Regression Detection

- **Threshold**: 5% slowdown
- **Action**: PR comment + warning
- **History**: 90 days of benchmark data

## 🎓 Learning Path

### Beginner

1. Read [QUICK_CI_SETUP.md](QUICK_CI_SETUP.md)
2. Run [scripts/verify_ci_setup.sh](scripts/verify_ci_setup.sh)
3. Review [.github/WORKFLOWS.md](.github/WORKFLOWS.md)

### Intermediate

1. Read [.github/CI_CD_GUIDE.md](.github/CI_CD_GUIDE.md)
2. Run [scripts/test_ci_locally.sh](scripts/test_ci_locally.sh)
3. Study [.github/workflows/ci.yml](.github/workflows/ci.yml)

### Advanced

1. Customize workflows
2. Add new benchmark positions
3. Optimize caching strategy
4. Contribute improvements

## 📦 File Structure

```
chess/
├── .github/
│   ├── workflows/
│   │   ├── ci.yml                    # CI workflow (450+ lines)
│   │   └── release.yml               # Release workflow (200+ lines)
│   ├── CI_CD_GUIDE.md                # Complete guide (400+ lines)
│   ├── WORKFLOWS.md                  # Visual diagrams (500+ lines)
│   └── README.md                     # GitHub Actions overview
├── scripts/
│   ├── benchmark.py                  # Python benchmark (300+ lines)
│   ├── benchmark.sh                  # Shell benchmark (150+ lines)
│   ├── benchmark.bat                 # Windows benchmark (150+ lines)
│   ├── compare_benchmarks.py         # Comparison tool (200+ lines)
│   ├── test_ci_locally.sh            # Local CI testing (150+ lines)
│   ├── verify_ci_setup.sh            # Setup verification (150+ lines)
│   ├── setup_scripts.sh              # Setup helper (30 lines)
│   └── README.md                     # Scripts documentation (400+ lines)
├── .clang-format                     # Formatting config (40 lines)
├── .clang-tidy                       # Static analysis config (30 lines)
├── .codecov.yml                      # Coverage config (25 lines)
├── LICENSE                           # MIT License (20 lines)
├── CONTRIBUTING.md                   # Contribution guide (200+ lines)
├── QUICK_CI_SETUP.md                 # Quick start (300+ lines)
├── CI_CD_SUMMARY.md                  # Implementation summary (600+ lines)
├── CI_CD_INDEX.md                    # This file
└── README.md                         # Main README (updated)

Total: 15+ files, 3,000+ lines of CI/CD infrastructure
```

## 🔗 External Resources

### GitHub Actions
- [Documentation](https://docs.github.com/en/actions)
- [Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
- [Marketplace](https://github.com/marketplace?type=actions)

### Tools
- [CMake](https://cmake.org/documentation/)
- [Codecov](https://docs.codecov.io/)
- [Clang-Format](https://clang.llvm.org/docs/ClangFormat.html)
- [Clang-Tidy](https://clang.llvm.org/extra/clang-tidy/)

### Chess Engine Resources
- [Chess Programming Wiki](https://www.chessprogramming.org/)
- [UCI Protocol](http://wbec-ridderkerk.nl/html/UCIProtocol.html)
- [Perft Results](https://www.chessprogramming.org/Perft_Results)

## ✅ Checklist

### Initial Setup
- [ ] Read QUICK_CI_SETUP.md
- [ ] Run verify_ci_setup.sh
- [ ] Update badges in README
- [ ] Push to GitHub
- [ ] Verify CI runs
- [ ] (Optional) Set up Codecov

### Before Each Push
- [ ] Run test_ci_locally.sh
- [ ] Check formatting
- [ ] Run benchmarks
- [ ] Review changes

### Before Each Release
- [ ] All tests passing
- [ ] Benchmarks stable
- [ ] Documentation updated
- [ ] Version tagged
- [ ] Release notes ready

## 🎉 Success Indicators

Your CI/CD is successful when:

1. ✅ All builds pass on all platforms
2. ✅ Tests verify correctness automatically
3. ✅ Benchmarks track performance trends
4. ✅ Regressions detected before merge
5. ✅ Releases automated and reliable
6. ✅ Contributors can test locally
7. ✅ Documentation comprehensive
8. ✅ Code quality maintained

## 📞 Getting Help

1. **Check documentation** in this index
2. **Review workflow logs** in GitHub Actions
3. **Run verification** with verify_ci_setup.sh
4. **Test locally** with test_ci_locally.sh
5. **Open an issue** if stuck

## 🤝 Contributing to CI/CD

Improvements welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

When modifying CI/CD:
1. Test locally first
2. Document changes
3. Update this index
4. Submit PR

---

**Version**: 1.0.0  
**Last Updated**: March 1, 2026  
**Maintainer**: Chess Engine Contributors  
**License**: MIT  

**Total Documentation**: 2,000+ lines  
**Total Scripts**: 1,000+ lines  
**Total Workflows**: 650+ lines  
**Total**: 3,650+ lines of CI/CD infrastructure

This is a **production-quality CI/CD pipeline** ready for professional chess engine development! 🚀
