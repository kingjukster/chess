# Quick CI/CD Setup Guide

Get your CI/CD pipeline running in 5 minutes!

## Prerequisites

- GitHub repository for your chess engine
- GitHub account with repository access

## Step 1: Update Repository Information

Edit `README.md` and replace the placeholder badges with your repository information:

```markdown
[![CI](https://github.com/YOUR_USERNAME/YOUR_REPO/workflows/CI/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/ci.yml)
[![Release](https://github.com/YOUR_USERNAME/YOUR_REPO/workflows/Release/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/release.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/YOUR_REPO)
```

Replace:
- `YOUR_USERNAME` with your GitHub username
- `YOUR_REPO` with your repository name

## Step 2: Push to GitHub

```bash
# Initialize git if not already done
git init

# Add all files
git add .

# Commit
git commit -m "Add CI/CD pipeline"

# Add remote (replace with your repository URL)
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git

# Push to main branch
git push -u origin main
```

## Step 3: Enable GitHub Actions

1. Go to your repository on GitHub
2. Click on the "Actions" tab
3. If prompted, click "I understand my workflows, go ahead and enable them"

The CI workflow will automatically run on your first push!

## Step 4: Set Up Code Coverage (Optional)

1. Go to https://codecov.io
2. Sign in with GitHub
3. Add your repository
4. Copy the upload token
5. Go to your GitHub repository → Settings → Secrets and variables → Actions
6. Click "New repository secret"
7. Name: `CODECOV_TOKEN`
8. Value: Paste your Codecov token
9. Click "Add secret"

## Step 5: Test Locally (Optional but Recommended)

Before pushing, test the CI pipeline locally:

```bash
# Make scripts executable (Unix/Linux/macOS)
chmod +x scripts/*.sh scripts/*.py

# Run local CI tests
./scripts/test_ci_locally.sh
```

On Windows:
```cmd
REM Run benchmark
scripts\benchmark.bat --engine build\Release\chess_engine.exe
```

## Step 6: Create Your First Release

Once CI is passing:

```bash
# Tag a release
git tag -a v1.0.0 -m "First release"
git push origin v1.0.0
```

The release workflow will automatically:
- Build binaries for Linux, Windows, and macOS
- Create a GitHub release
- Upload the binaries
- Generate release notes

## Verification

### Check CI Status

1. Go to your repository on GitHub
2. Click "Actions" tab
3. You should see workflows running or completed
4. Green checkmarks = success!

### Check Badges

The badges in your README should now show:
- CI: passing (green)
- Release: passing (green)
- Coverage: percentage (if configured)

## Troubleshooting

### CI Workflow Not Running

**Problem:** No workflows appear in the Actions tab

**Solution:**
1. Ensure `.github/workflows/` directory exists
2. Check that `ci.yml` and `release.yml` are present
3. Verify files are committed and pushed
4. Check GitHub Actions is enabled for your repository

### Build Fails on CI

**Problem:** CI shows red X, build fails

**Solution:**
1. Click on the failed workflow
2. Expand the failed job to see error details
3. Common issues:
   - Missing dependencies: Check CMakeLists.txt
   - Compiler errors: Test locally with same compiler
   - Test failures: Run perft tests locally

### Perft Tests Fail

**Problem:** Perft tests don't match expected values

**Solution:**
1. Run perft locally: `echo "perft 5" | ./build/chess_engine`
2. Compare with expected: 4,865,609 nodes
3. Debug move generation if incorrect
4. Ensure attack tables are initialized

### Release Workflow Doesn't Trigger

**Problem:** Pushing a tag doesn't create a release

**Solution:**
1. Verify tag format: `v1.0.0` (must start with 'v')
2. Check release.yml is in `.github/workflows/`
3. Ensure tag is pushed: `git push origin v1.0.0`
4. Check Actions tab for workflow run

### Coverage Upload Fails

**Problem:** Codecov badge shows "unknown"

**Solution:**
1. Verify `CODECOV_TOKEN` secret is set
2. Check token is correct (regenerate if needed)
3. Ensure Ubuntu GCC Debug build runs
4. Check Codecov.io for upload status

## Next Steps

### Customize Workflows

Edit `.github/workflows/ci.yml` to:
- Add more test positions
- Adjust benchmark depths
- Enable/disable specific checks
- Add custom build configurations

### Add More Tests

Create test files in a `tests/` directory:
- Unit tests for move generation
- Position test suites (EPD files)
- Tactical test positions
- Endgame test positions

### Improve Performance

Monitor benchmark results:
```bash
# Run benchmark
python3 scripts/benchmark.py --engine ./build/chess_engine --output baseline.json

# After changes
python3 scripts/benchmark.py --engine ./build/chess_engine --output new.json

# Compare
python3 scripts/compare_benchmarks.py baseline.json new.json
```

### Set Up Branch Protection

Protect your main branch:
1. Go to Settings → Branches
2. Add rule for `main`
3. Require status checks to pass:
   - ✓ build-and-test
   - ✓ benchmark
   - ✓ static-analysis
   - ✓ format-check
4. Require pull request reviews
5. Save changes

Now all changes must pass CI before merging!

## Resources

- **Full CI/CD Guide**: `.github/CI_CD_GUIDE.md`
- **Contributing Guide**: `CONTRIBUTING.md`
- **Scripts Documentation**: `scripts/README.md`
- **GitHub Actions Docs**: https://docs.github.com/en/actions
- **Codecov Docs**: https://docs.codecov.io/

## Getting Help

- Check workflow logs in Actions tab
- Review error messages carefully
- Test locally before pushing
- Open an issue if stuck

## Success Checklist

- [ ] Repository on GitHub
- [ ] Badges updated in README
- [ ] First commit pushed
- [ ] CI workflow running (green checkmark)
- [ ] Perft tests passing
- [ ] Benchmark completing
- [ ] (Optional) Codecov configured
- [ ] (Optional) First release created

Congratulations! Your CI/CD pipeline is now set up! 🎉
