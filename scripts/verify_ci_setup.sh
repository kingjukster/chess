#!/bin/bash
# Verify CI/CD setup is complete and correct

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================================================"
echo "CI/CD Setup Verification"
echo -e "========================================================================${NC}"
echo ""

ERRORS=0
WARNINGS=0

# Function to check file exists
check_file() {
    local file=$1
    local description=$2
    
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $description: $file"
        return 0
    else
        echo -e "${RED}✗${NC} $description: $file ${RED}(MISSING)${NC}"
        ERRORS=$((ERRORS + 1))
        return 1
    fi
}

# Function to check directory exists
check_dir() {
    local dir=$1
    local description=$2
    
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $description: $dir"
        return 0
    else
        echo -e "${RED}✗${NC} $description: $dir ${RED}(MISSING)${NC}"
        ERRORS=$((ERRORS + 1))
        return 1
    fi
}

# Function to check file is executable
check_executable() {
    local file=$1
    local description=$2
    
    if [ -x "$file" ]; then
        echo -e "${GREEN}✓${NC} $description: $file ${GREEN}(executable)${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} $description: $file ${YELLOW}(not executable)${NC}"
        echo -e "   Run: chmod +x $file"
        WARNINGS=$((WARNINGS + 1))
        return 1
    fi
}

echo "Checking GitHub Actions Workflows..."
echo "--------------------------------------------------------------------"
check_dir ".github/workflows" "Workflows directory"
check_file ".github/workflows/ci.yml" "CI workflow"
check_file ".github/workflows/release.yml" "Release workflow"
echo ""

echo "Checking Documentation..."
echo "--------------------------------------------------------------------"
check_file ".github/CI_CD_GUIDE.md" "CI/CD guide"
check_file ".github/WORKFLOWS.md" "Workflows visual guide"
check_file "CONTRIBUTING.md" "Contributing guidelines"
check_file "QUICK_CI_SETUP.md" "Quick setup guide"
check_file "CI_CD_SUMMARY.md" "CI/CD summary"
check_file "LICENSE" "License file"
echo ""

echo "Checking Configuration Files..."
echo "--------------------------------------------------------------------"
check_file ".clang-format" "Clang-format config"
check_file ".clang-tidy" "Clang-tidy config"
check_file ".codecov.yml" "Codecov config"
echo ""

echo "Checking Scripts..."
echo "--------------------------------------------------------------------"
check_dir "scripts" "Scripts directory"
check_file "scripts/benchmark.py" "Python benchmark script"
check_file "scripts/benchmark.sh" "Shell benchmark script"
check_file "scripts/benchmark.bat" "Windows benchmark script"
check_file "scripts/compare_benchmarks.py" "Benchmark comparison script"
check_file "scripts/test_ci_locally.sh" "Local CI test script"
check_file "scripts/setup_scripts.sh" "Setup scripts helper"
check_file "scripts/README.md" "Scripts documentation"
echo ""

echo "Checking Script Permissions (Unix/Linux/macOS)..."
echo "--------------------------------------------------------------------"
if [ "$(uname)" != "MINGW"* ] && [ "$(uname)" != "MSYS"* ]; then
    check_executable "scripts/benchmark.sh" "Shell benchmark"
    check_executable "scripts/benchmark.py" "Python benchmark"
    check_executable "scripts/compare_benchmarks.py" "Benchmark comparison"
    check_executable "scripts/test_ci_locally.sh" "Local CI test"
    check_executable "scripts/setup_scripts.sh" "Setup scripts"
else
    echo -e "${YELLOW}⚠${NC} Skipping executable check (Windows detected)"
fi
echo ""

echo "Checking README Updates..."
echo "--------------------------------------------------------------------"
if grep -q "CI/CD Pipeline" README.md 2>/dev/null; then
    echo -e "${GREEN}✓${NC} README contains CI/CD section"
else
    echo -e "${YELLOW}⚠${NC} README may need CI/CD section"
    WARNINGS=$((WARNINGS + 1))
fi

if grep -q "workflows/ci.yml" README.md 2>/dev/null; then
    echo -e "${GREEN}✓${NC} README contains CI badge"
else
    echo -e "${YELLOW}⚠${NC} README may need CI badge (update YOUR_USERNAME/YOUR_REPO)"
    WARNINGS=$((WARNINGS + 1))
fi
echo ""

echo "Checking Build System..."
echo "--------------------------------------------------------------------"
if [ -f "CMakeLists.txt" ]; then
    echo -e "${GREEN}✓${NC} CMakeLists.txt exists"
else
    echo -e "${RED}✗${NC} CMakeLists.txt missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -d "build" ]; then
    echo -e "${GREEN}✓${NC} Build directory exists"
    if [ -f "build/chess_engine" ] || [ -f "build/Release/chess_engine.exe" ]; then
        echo -e "${GREEN}✓${NC} Engine binary found"
    else
        echo -e "${YELLOW}⚠${NC} Engine not built yet (run: cmake -B build && cmake --build build)"
        WARNINGS=$((WARNINGS + 1))
    fi
else
    echo -e "${YELLOW}⚠${NC} Build directory not found (run: mkdir build && cd build && cmake ..)"
    WARNINGS=$((WARNINGS + 1))
fi
echo ""

echo "Checking Optional Tools..."
echo "--------------------------------------------------------------------"
if command -v clang-format &> /dev/null; then
    echo -e "${GREEN}✓${NC} clang-format installed"
else
    echo -e "${YELLOW}⚠${NC} clang-format not found (optional, for formatting checks)"
fi

if command -v clang-tidy &> /dev/null; then
    echo -e "${GREEN}✓${NC} clang-tidy installed"
else
    echo -e "${YELLOW}⚠${NC} clang-tidy not found (optional, for static analysis)"
fi

if command -v python3 &> /dev/null; then
    echo -e "${GREEN}✓${NC} Python 3 installed"
    PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
    echo -e "   Version: $PYTHON_VERSION"
else
    echo -e "${RED}✗${NC} Python 3 not found (required for benchmark scripts)"
    ERRORS=$((ERRORS + 1))
fi

if command -v cmake &> /dev/null; then
    echo -e "${GREEN}✓${NC} CMake installed"
    CMAKE_VERSION=$(cmake --version 2>&1 | head -1 | cut -d' ' -f3)
    echo -e "   Version: $CMAKE_VERSION"
else
    echo -e "${RED}✗${NC} CMake not found (required for building)"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo -e "${BLUE}========================================================================"
echo "Summary"
echo -e "========================================================================${NC}"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ Perfect! CI/CD setup is complete and correct.${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Update badges in README.md with your repository info"
    echo "2. Push to GitHub: git add . && git commit -m 'Add CI/CD' && git push"
    echo "3. Check GitHub Actions tab to see workflows run"
    echo "4. (Optional) Set up Codecov token for coverage reporting"
    echo ""
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠ Setup mostly complete with $WARNINGS warning(s).${NC}"
    echo ""
    echo "Warnings are optional issues that won't prevent CI from working."
    echo "Review the warnings above and fix if needed."
    echo ""
    exit 0
else
    echo -e "${RED}✗ Setup incomplete: $ERRORS error(s), $WARNINGS warning(s).${NC}"
    echo ""
    echo "Please fix the errors above before proceeding."
    echo "Missing files may indicate incomplete installation."
    echo ""
    exit 1
fi
