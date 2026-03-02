#!/usr/bin/env python3
"""
Position Analyzer for Chess Engine
Analyzes positions and provides detailed evaluation breakdown
"""

import subprocess
import sys
import os
import argparse
import json
import time


class ChessAnalyzer:
    """Analyzes chess positions using the engine"""
    
    def __init__(self, engine_path):
        self.engine_path = engine_path
        self.process = None
        
    def start(self):
        """Start the engine"""
        self.process = subprocess.Popen(
            [self.engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1
        )
        
        # Initialize UCI
        self.send("uci")
        self.wait_for("uciok")
        self.send("isready")
        self.wait_for("readyok")
        
    def stop(self):
        """Stop the engine"""
        if self.process:
            self.send("quit")
            self.process.wait(timeout=5)
            
    def send(self, command):
        """Send command to engine"""
        if self.process:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
            
    def wait_for(self, text, timeout=10):
        """Wait for specific text in output"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            line = self.process.stdout.readline().strip()
            if text in line:
                return line
        raise TimeoutError(f"Timeout waiting for '{text}'")
        
    def display_position(self, fen=None):
        """Display the current position"""
        if fen:
            self.send("ucinewgame")
            self.send(f"position fen {fen}")
        
        self.send("d")
        
        # Read and print board display
        lines = []
        while True:
            line = self.process.stdout.readline().strip()
            if not line:
                break
            lines.append(line)
            if "Evaluation:" in line:
                # Read a few more lines
                for _ in range(2):
                    extra = self.process.stdout.readline().strip()
                    if extra:
                        lines.append(extra)
                break
                
        return "\n".join(lines)
        
    def analyze(self, fen=None, depth=20, multipv=1):
        """Analyze a position"""
        if fen:
            self.send("ucinewgame")
            self.send(f"position fen {fen}")
        
        # Set MultiPV if requested
        if multipv > 1:
            self.send(f"setoption name MultiPV value {multipv}")
            
        # Start analysis
        self.send(f"go depth {depth}")
        
        # Collect analysis info
        analysis = {
            "depth": 0,
            "nodes": 0,
            "time": 0,
            "nps": 0,
            "pv_lines": [],
            "best_move": None
        }
        
        pv_data = {}
        
        while True:
            line = self.process.stdout.readline().strip()
            
            if line.startswith("info"):
                # Parse info line
                parts = line.split()
                pv_num = 1
                
                try:
                    if "multipv" in parts:
                        idx = parts.index("multipv")
                        pv_num = int(parts[idx + 1])
                        
                    if "depth" in parts:
                        idx = parts.index("depth")
                        depth_val = int(parts[idx + 1])
                        analysis["depth"] = max(analysis["depth"], depth_val)
                        
                    if "nodes" in parts:
                        idx = parts.index("nodes")
                        analysis["nodes"] = int(parts[idx + 1])
                        
                    if "time" in parts:
                        idx = parts.index("time")
                        analysis["time"] = int(parts[idx + 1])
                        
                    if "nps" in parts:
                        idx = parts.index("nps")
                        analysis["nps"] = int(parts[idx + 1])
                        
                    if "score" in parts:
                        idx = parts.index("score")
                        score_type = parts[idx + 1]
                        score_value = parts[idx + 2]
                        
                        if score_type == "cp":
                            score = int(score_value) / 100.0
                        elif score_type == "mate":
                            mate_in = int(score_value)
                            score = f"Mate in {abs(mate_in)}"
                        else:
                            score = score_value
                            
                        if pv_num not in pv_data:
                            pv_data[pv_num] = {}
                        pv_data[pv_num]["score"] = score
                        
                    if "pv" in parts:
                        idx = parts.index("pv")
                        pv = parts[idx + 1:]
                        
                        if pv_num not in pv_data:
                            pv_data[pv_num] = {}
                        pv_data[pv_num]["pv"] = pv
                        
                except (ValueError, IndexError):
                    pass
                    
            elif line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    analysis["best_move"] = parts[1]
                break
                
        # Convert pv_data to list
        for pv_num in sorted(pv_data.keys()):
            data = pv_data[pv_num]
            if "pv" in data and "score" in data:
                analysis["pv_lines"].append({
                    "num": pv_num,
                    "score": data["score"],
                    "pv": data["pv"]
                })
                
        return analysis
        
    def visualize_board(self, fen):
        """Visualize board in terminal"""
        print("\n" + "=" * 80)
        print("POSITION ANALYSIS")
        print("=" * 80)
        print()
        
        display = self.display_position(fen)
        print(display)
        print()
        
    def print_analysis(self, analysis):
        """Print analysis results"""
        print("=" * 80)
        print("ANALYSIS RESULTS")
        print("=" * 80)
        print(f"Depth: {analysis['depth']}")
        print(f"Nodes: {analysis['nodes']:,}")
        print(f"Time: {analysis['time']} ms")
        print(f"NPS: {analysis['nps']:,}")
        print()
        
        if analysis["pv_lines"]:
            print("Principal Variations:")
            print("-" * 80)
            
            for pv_line in analysis["pv_lines"]:
                score = pv_line["score"]
                if isinstance(score, float):
                    score_str = f"{score:+.2f}"
                else:
                    score_str = str(score)
                    
                pv = " ".join(pv_line["pv"][:10])  # Show first 10 moves
                if len(pv_line["pv"]) > 10:
                    pv += " ..."
                    
                print(f"{pv_line['num']:2}. [{score_str:>8}] {pv}")
                
        print("=" * 80)
        
    def export_json(self, analysis, output_file):
        """Export analysis to JSON"""
        with open(output_file, 'w') as f:
            json.dump(analysis, f, indent=2)
        print(f"\nAnalysis exported to {output_file}")
        
    def export_text(self, fen, analysis, output_file):
        """Export analysis to text file"""
        with open(output_file, 'w') as f:
            f.write("CHESS POSITION ANALYSIS\n")
            f.write("=" * 80 + "\n\n")
            f.write(f"FEN: {fen}\n\n")
            
            f.write("Board:\n")
            display = self.display_position(fen)
            f.write(display + "\n\n")
            
            f.write(f"Depth: {analysis['depth']}\n")
            f.write(f"Nodes: {analysis['nodes']:,}\n")
            f.write(f"Time: {analysis['time']} ms\n")
            f.write(f"NPS: {analysis['nps']:,}\n\n")
            
            if analysis["pv_lines"]:
                f.write("Principal Variations:\n")
                f.write("-" * 80 + "\n")
                
                for pv_line in analysis["pv_lines"]:
                    score = pv_line["score"]
                    if isinstance(score, float):
                        score_str = f"{score:+.2f}"
                    else:
                        score_str = str(score)
                        
                    pv = " ".join(pv_line["pv"])
                    f.write(f"{pv_line['num']:2}. [{score_str:>8}] {pv}\n")
                    
        print(f"\nAnalysis exported to {output_file}")


def main():
    parser = argparse.ArgumentParser(description="Analyze chess positions")
    parser.add_argument("engine", help="Path to chess engine executable")
    parser.add_argument("-f", "--fen", 
                       default="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                       help="FEN string of position to analyze")
    parser.add_argument("-d", "--depth", type=int, default=20,
                       help="Search depth (default: 20)")
    parser.add_argument("-m", "--multipv", type=int, default=3,
                       help="Number of principal variations (default: 3)")
    parser.add_argument("--json", help="Export analysis to JSON file")
    parser.add_argument("--text", help="Export analysis to text file")
    parser.add_argument("--no-display", action="store_true",
                       help="Don't display board visualization")
    
    args = parser.parse_args()
    
    # Validate engine
    if not os.path.exists(args.engine):
        print(f"Error: Engine not found: {args.engine}")
        sys.exit(1)
        
    # Create analyzer
    analyzer = ChessAnalyzer(args.engine)
    
    try:
        analyzer.start()
        
        # Display position
        if not args.no_display:
            analyzer.visualize_board(args.fen)
            
        # Analyze
        print(f"Analyzing position (depth={args.depth}, multipv={args.multipv})...")
        analysis = analyzer.analyze(args.fen, args.depth, args.multipv)
        
        # Print results
        analyzer.print_analysis(analysis)
        
        # Export if requested
        if args.json:
            analyzer.export_json(analysis, args.json)
            
        if args.text:
            analyzer.export_text(args.fen, analysis, args.text)
            
    finally:
        analyzer.stop()


if __name__ == "__main__":
    main()
