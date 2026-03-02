#!/bin/bash
# Chess Engine Benchmark Script (Shell version)
# Simple benchmark runner for quick performance checks

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
ENGINE="./build/chess_engine"
DEPTH=8

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --engine)
            ENGINE="$2"
            shift 2
            ;;
        --depth)
            DEPTH="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --engine PATH    Path to chess engine (default: ./build/chess_engine)"
            echo "  --depth N        Search depth (default: 8)"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check if engine exists
if [ ! -f "$ENGINE" ]; then
    echo -e "${RED}Error: Engine not found: $ENGINE${NC}"
    exit 1
fi

echo "========================================================================"
echo "Chess Engine Benchmark"
echo "========================================================================"
echo "Engine: $ENGINE"
echo "Depth:  $DEPTH"
echo ""

# Benchmark positions
declare -A POSITIONS
POSITIONS["startpos"]="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
POSITIONS["kiwipete"]="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
POSITIONS["endgame"]="8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
POSITIONS["tactics"]="r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
POSITIONS["promotion"]="rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
POSITIONS["middlegame"]="r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"

total_nodes=0
total_time=0

# Run benchmark for each position
for pos_name in "${!POSITIONS[@]}"; do
    fen="${POSITIONS[$pos_name]}"
    
    echo -e "${BLUE}Position: $pos_name${NC}"
    
    # Create UCI commands
    commands="uci
isready
ucinewgame
position fen $fen
go depth $DEPTH
quit"
    
    # Run engine and capture output
    start_time=$(date +%s%N)
    output=$(echo "$commands" | "$ENGINE" 2>&1)
    end_time=$(date +%s%N)
    
    # Calculate elapsed time in milliseconds
    elapsed_ns=$((end_time - start_time))
    elapsed_ms=$((elapsed_ns / 1000000))
    
    # Extract nodes from output
    nodes=$(echo "$output" | grep "info.*nodes" | tail -1 | sed -n 's/.*nodes \([0-9]*\).*/\1/p')
    
    if [ -z "$nodes" ]; then
        nodes=0
    fi
    
    # Calculate NPS
    if [ $elapsed_ms -gt 0 ]; then
        nps=$((nodes * 1000 / elapsed_ms))
    else
        nps=0
    fi
    
    total_nodes=$((total_nodes + nodes))
    total_time=$((total_time + elapsed_ms))
    
    # Format numbers with commas
    nodes_formatted=$(printf "%'d" $nodes)
    nps_formatted=$(printf "%'d" $nps)
    
    echo -e "  ${GREEN}✓${NC} $nps_formatted nodes/sec (${nodes_formatted} nodes in ${elapsed_ms}ms)"
    echo ""
done

# Calculate average NPS
if [ $total_time -gt 0 ]; then
    avg_nps=$((total_nodes * 1000 / total_time))
else
    avg_nps=0
fi

total_nodes_formatted=$(printf "%'d" $total_nodes)
avg_nps_formatted=$(printf "%'d" $avg_nps)
total_time_sec=$(echo "scale=2; $total_time / 1000" | bc)

echo "========================================================================"
echo "Summary"
echo "========================================================================"
echo "Total nodes:    $total_nodes_formatted"
echo "Total time:     ${total_time_sec}s"
echo "Average NPS:    $avg_nps_formatted"
echo "========================================================================"

# Run perft tests
echo ""
echo "Perft Tests"
echo "------------------------------------------------------------------------"

perft_tests=(1:20 2:400 3:8902 4:197281 5:4865609)

for test in "${perft_tests[@]}"; do
    depth="${test%%:*}"
    expected="${test##*:}"
    
    commands="uci
perft $depth
quit"
    
    start_time=$(date +%s%N)
    output=$(echo "$commands" | "$ENGINE" 2>&1)
    end_time=$(date +%s%N)
    
    elapsed_ns=$((end_time - start_time))
    elapsed_ms=$((elapsed_ns / 1000000))
    
    # Extract nodes from perft output
    nodes=$(echo "$output" | grep "perft $depth" | sed -n "s/perft $depth \([0-9]*\)/\1/p")
    
    if [ -z "$nodes" ]; then
        nodes=0
    fi
    
    # Calculate NPS
    if [ $elapsed_ms -gt 0 ]; then
        nps=$((nodes * 1000 / elapsed_ms))
    else
        nps=0
    fi
    
    nodes_formatted=$(printf "%'d" $nodes)
    nps_formatted=$(printf "%'d" $nps)
    
    # Check correctness
    if [ "$nodes" -eq "$expected" ]; then
        status="${GREEN}✓${NC}"
    else
        status="${RED}✗${NC}"
    fi
    
    echo -e "Perft $depth: $nodes_formatted nodes in ${elapsed_ms}ms ($nps_formatted nps) $status"
done

echo ""
echo -e "${GREEN}Benchmark complete!${NC}"
