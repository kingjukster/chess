#!/usr/bin/env python3
"""
Chess Engine Benchmark Script

Runs standardized benchmark positions to measure engine performance.
Outputs nodes per second (NPS) and other performance metrics.
"""

import subprocess
import time
import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Tuple

# Standard benchmark positions
BENCHMARK_POSITIONS = {
    "startpos": {
        "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "depth": 8,
        "description": "Starting position"
    },
    "kiwipete": {
        "fen": "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "depth": 7,
        "description": "Kiwipete position (complex middlegame)"
    },
    "endgame": {
        "fen": "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "depth": 10,
        "description": "Complex endgame position"
    },
    "tactics": {
        "fen": "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "depth": 7,
        "description": "Tactical position"
    },
    "promotion": {
        "fen": "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "depth": 7,
        "description": "Position with promotion threats"
    },
    "middlegame": {
        "fen": "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "depth": 7,
        "description": "Balanced middlegame"
    }
}


class ChessEngineBenchmark:
    def __init__(self, engine_path: str, verbose: bool = False):
        self.engine_path = Path(engine_path)
        self.verbose = verbose
        
        if not self.engine_path.exists():
            raise FileNotFoundError(f"Engine not found: {engine_path}")
    
    def run_position(self, fen: str, depth: int) -> Tuple[int, float, int]:
        """
        Run benchmark on a single position.
        
        Returns:
            (nodes, time_ms, nps)
        """
        commands = [
            "uci",
            "isready",
            "ucinewgame",
            f"position fen {fen}",
            f"go depth {depth}",
            "quit"
        ]
        
        input_str = "\n".join(commands) + "\n"
        
        if self.verbose:
            print(f"Running: depth {depth}, FEN: {fen[:50]}...")
        
        start_time = time.time()
        
        try:
            result = subprocess.run(
                [str(self.engine_path)],
                input=input_str,
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout
            )
        except subprocess.TimeoutExpired:
            print(f"Warning: Position timed out after 300 seconds")
            return 0, 0, 0
        
        end_time = time.time()
        elapsed_ms = (end_time - start_time) * 1000
        
        # Parse output for nodes
        nodes = 0
        for line in result.stdout.split('\n'):
            if 'info' in line and 'nodes' in line:
                parts = line.split()
                try:
                    nodes_idx = parts.index('nodes')
                    if nodes_idx + 1 < len(parts):
                        nodes = int(parts[nodes_idx + 1])
                except (ValueError, IndexError):
                    pass
        
        nps = int(nodes / (elapsed_ms / 1000)) if elapsed_ms > 0 else 0
        
        if self.verbose:
            print(f"  Nodes: {nodes:,}, Time: {elapsed_ms:.0f}ms, NPS: {nps:,}")
        
        return nodes, elapsed_ms, nps
    
    def run_benchmark(self) -> Dict:
        """
        Run full benchmark suite.
        
        Returns:
            Dictionary with benchmark results
        """
        print("=" * 70)
        print("Chess Engine Benchmark")
        print("=" * 70)
        print(f"Engine: {self.engine_path}")
        print()
        
        results = {
            "engine": str(self.engine_path),
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "positions": {},
            "total_nodes": 0,
            "total_time_ms": 0,
            "average_nps": 0
        }
        
        total_nodes = 0
        total_time = 0
        
        for pos_name, pos_data in BENCHMARK_POSITIONS.items():
            print(f"Position: {pos_name} - {pos_data['description']}")
            
            nodes, time_ms, nps = self.run_position(
                pos_data['fen'],
                pos_data['depth']
            )
            
            results["positions"][pos_name] = {
                "fen": pos_data['fen'],
                "depth": pos_data['depth'],
                "nodes": nodes,
                "time_ms": round(time_ms, 2),
                "nps": nps
            }
            
            total_nodes += nodes
            total_time += time_ms
            
            print(f"  ✓ {nps:,} nodes/sec")
            print()
        
        results["total_nodes"] = total_nodes
        results["total_time_ms"] = round(total_time, 2)
        results["average_nps"] = int(total_nodes / (total_time / 1000)) if total_time > 0 else 0
        
        print("=" * 70)
        print("Summary")
        print("=" * 70)
        print(f"Total nodes:    {total_nodes:,}")
        print(f"Total time:     {total_time/1000:.2f}s")
        print(f"Average NPS:    {results['average_nps']:,}")
        print("=" * 70)
        
        return results
    
    def run_perft_benchmark(self) -> Dict:
        """
        Run perft benchmark for move generation speed.
        
        Returns:
            Dictionary with perft results
        """
        print("\nPerft Benchmark")
        print("-" * 70)
        
        perft_tests = [
            (1, 20),
            (2, 400),
            (3, 8902),
            (4, 197281),
            (5, 4865609),
        ]
        
        results = {}
        
        for depth, expected_nodes in perft_tests:
            commands = [
                "uci",
                f"perft {depth}",
                "quit"
            ]
            
            input_str = "\n".join(commands) + "\n"
            
            start_time = time.time()
            
            try:
                result = subprocess.run(
                    [str(self.engine_path)],
                    input=input_str,
                    capture_output=True,
                    text=True,
                    timeout=60
                )
            except subprocess.TimeoutExpired:
                print(f"Perft {depth}: TIMEOUT")
                continue
            
            end_time = time.time()
            elapsed_ms = (end_time - start_time) * 1000
            
            # Parse perft output
            nodes = 0
            for line in result.stdout.split('\n'):
                if f'perft {depth}' in line:
                    parts = line.split()
                    if len(parts) >= 3:
                        try:
                            nodes = int(parts[2])
                        except ValueError:
                            pass
            
            nps = int(nodes / (elapsed_ms / 1000)) if elapsed_ms > 0 else 0
            correct = nodes == expected_nodes
            
            results[f"depth_{depth}"] = {
                "nodes": nodes,
                "expected": expected_nodes,
                "time_ms": round(elapsed_ms, 2),
                "nps": nps,
                "correct": correct
            }
            
            status = "✓" if correct else "✗"
            print(f"Perft {depth}: {nodes:,} nodes in {elapsed_ms:.0f}ms "
                  f"({nps:,} nps) {status}")
        
        return results


def main():
    parser = argparse.ArgumentParser(
        description="Benchmark chess engine performance"
    )
    parser.add_argument(
        "--engine",
        required=True,
        help="Path to chess engine executable"
    )
    parser.add_argument(
        "--output",
        help="Output JSON file for results"
    )
    parser.add_argument(
        "--perft",
        action="store_true",
        help="Run perft benchmark"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Verbose output"
    )
    
    args = parser.parse_args()
    
    try:
        benchmark = ChessEngineBenchmark(args.engine, args.verbose)
        
        # Run main benchmark
        results = benchmark.run_benchmark()
        
        # Run perft if requested
        if args.perft:
            perft_results = benchmark.run_perft_benchmark()
            results["perft"] = perft_results
        
        # Save results
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"\nResults saved to: {args.output}")
        
        return 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
