# Contributing to Chess Engine

Thank you for your interest in contributing to this chess engine project!

## Development Setup

1. **Clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/YOUR_REPO.git
   cd YOUR_REPO
   ```

2. **Install dependencies**
   - CMake 3.10+
   - C++17 compatible compiler (GCC 11+, Clang 14+, MSVC 2019+)
   - Python 3.7+ (for benchmarking scripts)

3. **Build the project**
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

## Code Style

This project follows a consistent code style enforced by clang-format and clang-tidy.

- **Formatting**: Run `clang-format` before committing
  ```bash
  find . -name '*.cpp' -o -name '*.h' | xargs clang-format -i
  ```

- **Linting**: Check code with `clang-tidy`
  ```bash
  cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  clang-tidy -p build <file.cpp>
  ```

## Testing

Before submitting a pull request:

1. **Run perft tests** to verify move generation correctness
   ```bash
   ./build/chess_engine
   perft 1
   perft 2
   perft 3
   perft 4
   perft 5
   ```

2. **Run UCI tests** to verify protocol compliance
   ```bash
   echo "uci" | ./build/chess_engine
   echo "isready" | ./build/chess_engine
   ```

3. **Run benchmarks** to check for performance regressions
   ```bash
   python3 scripts/benchmark.py --engine ./build/chess_engine
   ```

## Pull Request Process

1. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Write clear, concise commit messages
   - Follow the existing code style
   - Add tests if applicable
   - Update documentation if needed

3. **Test your changes**
   - Ensure all tests pass
   - Run benchmarks to check for regressions
   - Test on multiple platforms if possible

4. **Submit a pull request**
   - Provide a clear description of your changes
   - Reference any related issues
   - Wait for CI checks to pass
   - Address any review feedback

## Commit Message Guidelines

Use conventional commit format:

- `feat:` New feature
- `fix:` Bug fix
- `perf:` Performance improvement
- `refactor:` Code refactoring
- `docs:` Documentation changes
- `test:` Test additions or changes
- `chore:` Build process or auxiliary tool changes

Examples:
```
feat: add late move reduction to search
fix: correct en passant detection in move generation
perf: optimize NNUE accumulator updates
```

## Performance Considerations

When making changes that affect performance:

1. Run benchmarks before and after your changes
2. Document any performance impact in your PR
3. Significant regressions (>5%) require justification
4. Use the comparison script:
   ```bash
   python3 scripts/compare_benchmarks.py old.json new.json
   ```

## Areas for Contribution

### High Priority
- NNUE network training and weight optimization
- Advanced search techniques (LMR, null move pruning, etc.)
- Time management improvements
- Opening book integration
- Endgame tablebase support

### Medium Priority
- UCI option enhancements
- Move ordering improvements
- Transposition table optimization
- Multi-threaded search

### Low Priority
- Code documentation
- Test coverage improvements
- Build system enhancements
- Platform-specific optimizations

## Questions?

Feel free to open an issue for:
- Bug reports
- Feature requests
- Questions about the codebase
- Discussion of design decisions

## Code of Conduct

- Be respectful and constructive
- Focus on the code, not the person
- Welcome newcomers and help them learn
- Assume good intentions

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
