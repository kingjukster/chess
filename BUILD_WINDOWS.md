# Building on Windows

## Option 1: Install CMake (Recommended)

1. **Download CMake:**
   - Go to https://cmake.org/download/
   - Download "Windows x64 Installer"
   - Run installer, select "Add CMake to system PATH"

2. **Build:**
   ```powershell
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Run:**
   ```powershell
   .\Release\chess_engine.exe
   ```

## Option 2: Use Visual Studio

1. **Open Visual Studio:**
   - File → Open → CMake...
   - Select the `CMakeLists.txt` file

2. **Build:**
   - Build → Build All (or Ctrl+Shift+B)

3. **Run:**
   - Debug → Start Without Debugging (or Ctrl+F5)

## Option 3: Manual Compilation (No CMake)

If you have a C++ compiler (MSVC, MinGW, Clang):

```powershell
# Using MSVC (Visual Studio)
cl /EHsc /std:c++17 /I. ^
   board\position.cpp ^
   movegen\attacks.cpp movegen\movegen.cpp ^
   eval\classic_eval.cpp ^
   nnue\nnue_eval.cpp nnue\nnue_loader.cpp ^
   search\search.cpp ^
   uci\uci.cpp ^
   main.cpp ^
   /Fe:chess_engine.exe

# Using MinGW
g++ -std=c++17 -O3 -I. ^
    board/position.cpp ^
    movegen/attacks.cpp movegen/movegen.cpp ^
    eval/classic_eval.cpp ^
    nnue/nnue_eval.cpp nnue/nnue_loader.cpp ^
    search/search.cpp ^
    uci/uci.cpp ^
    main.cpp ^
    -o chess_engine.exe
```

## Quick Test Without Building

You can test the training pipeline without building the engine:

```powershell
cd training
python generate_synthetic_data.py --output test_data.bin --num-positions 1000
python train_nnue.py --data test_data.bin --epochs 2 --output test_model.pth
python export_weights.py --model test_model.pth --output ../test_weights.bin
```

This verifies the training pipeline works!

