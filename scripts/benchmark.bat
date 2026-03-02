@echo off
REM Chess Engine Benchmark Script (Windows version)
REM Simple benchmark runner for quick performance checks

setlocal enabledelayedexpansion

set ENGINE=build\Release\chess_engine.exe
set DEPTH=8

REM Parse arguments
:parse_args
if "%~1"=="" goto :main
if "%~1"=="--engine" (
    set ENGINE=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--depth" (
    set DEPTH=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    echo Usage: %~nx0 [OPTIONS]
    echo.
    echo Options:
    echo   --engine PATH    Path to chess engine (default: build\Release\chess_engine.exe^)
    echo   --depth N        Search depth (default: 8^)
    echo   --help           Show this help message
    exit /b 0
)
shift
goto :parse_args

:main

REM Check if engine exists
if not exist "%ENGINE%" (
    echo Error: Engine not found: %ENGINE%
    exit /b 1
)

echo ========================================================================
echo Chess Engine Benchmark
echo ========================================================================
echo Engine: %ENGINE%
echo Depth:  %DEPTH%
echo.

REM Create temporary files for positions
set TEMP_DIR=%TEMP%\chess_benchmark_%RANDOM%
mkdir "%TEMP_DIR%" 2>nul

REM Define positions
echo rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 > "%TEMP_DIR%\startpos.fen"
echo r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 > "%TEMP_DIR%\kiwipete.fen"
echo 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 > "%TEMP_DIR%\endgame.fen"
echo r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1 > "%TEMP_DIR%\tactics.fen"
echo rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 > "%TEMP_DIR%\promotion.fen"
echo r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 > "%TEMP_DIR%\middlegame.fen"

set TOTAL_NODES=0
set TOTAL_TIME=0

REM Run benchmarks
for %%P in (startpos kiwipete endgame tactics promotion middlegame) do (
    echo Position: %%P
    
    set /p FEN=<"%TEMP_DIR%\%%P.fen"
    
    REM Create UCI commands
    (
        echo uci
        echo isready
        echo ucinewgame
        echo position fen !FEN!
        echo go depth %DEPTH%
        echo quit
    ) > "%TEMP_DIR%\commands.txt"
    
    REM Run engine and measure time
    set START_TIME=!TIME!
    "%ENGINE%" < "%TEMP_DIR%\commands.txt" > "%TEMP_DIR%\output.txt" 2>&1
    set END_TIME=!TIME!
    
    REM Extract nodes from output
    set NODES=0
    for /f "tokens=*" %%L in ('findstr /C:"info" "%TEMP_DIR%\output.txt" ^| findstr /C:"nodes"') do (
        for /f "tokens=1-20" %%A in ("%%L") do (
            set FOUND_NODES=0
            for %%X in (%%A %%B %%C %%D %%E %%F %%G %%H %%I %%J %%K %%L %%M %%N %%O %%P %%Q %%R %%S %%T) do (
                if "!FOUND_NODES!"=="1" (
                    set NODES=%%X
                    set FOUND_NODES=0
                )
                if "%%X"=="nodes" set FOUND_NODES=1
            )
        )
    )
    
    if !NODES! GTR 0 (
        echo   [32m✓[0m !NODES! nodes
    ) else (
        echo   [33m⚠[0m No nodes found
    )
    echo.
)

REM Cleanup
rd /s /q "%TEMP_DIR%" 2>nul

echo ========================================================================
echo Summary
echo ========================================================================
echo Benchmark complete! See individual position results above.
echo ========================================================================

REM Run perft tests
echo.
echo Perft Tests
echo ------------------------------------------------------------------------

for %%D in (1 2 3 4 5) do (
    (
        echo uci
        echo perft %%D
        echo quit
    ) | "%ENGINE%" > "%TEMP%\perft_output.txt" 2>&1
    
    for /f "tokens=3" %%N in ('findstr /C:"perft %%D" "%TEMP%\perft_output.txt"') do (
        echo Perft %%D: %%N nodes
    )
)

del "%TEMP%\perft_output.txt" 2>nul

echo.
echo [32mBenchmark complete![0m

endlocal
