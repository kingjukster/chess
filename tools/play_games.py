#!/usr/bin/env python3
"""
Game Player Script for Chess Engine Testing
Plays games against other engines and generates statistics
"""

import subprocess
import sys
import os
import argparse
import json
import time
from datetime import datetime
from pathlib import Path


class Engine:
    """UCI chess engine wrapper"""
    
    def __init__(self, path, name=None):
        self.path = path
        self.name = name or Path(path).stem
        self.process = None
        
    def start(self):
        """Start the engine process"""
        self.process = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1
        )
        
        # Initialize UCI
        self.send("uci")
        self.wait_for("uciok")
        
    def stop(self):
        """Stop the engine process"""
        if self.process:
            self.send("quit")
            self.process.wait(timeout=5)
            self.process = None
            
    def send(self, command):
        """Send command to engine"""
        if self.process:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
            
    def wait_for(self, text, timeout=10):
        """Wait for specific text in engine output"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            line = self.process.stdout.readline().strip()
            if text in line:
                return line
        raise TimeoutError(f"Timeout waiting for '{text}'")
        
    def get_move(self, position, wtime=None, btime=None, depth=None):
        """Get best move for position"""
        self.send("ucinewgame")
        self.send(f"position {position}")
        
        go_cmd = "go"
        if depth:
            go_cmd += f" depth {depth}"
        elif wtime and btime:
            go_cmd += f" wtime {wtime} btime {btime}"
        else:
            go_cmd += " movetime 1000"
            
        self.send(go_cmd)
        
        # Wait for bestmove
        while True:
            line = self.process.stdout.readline().strip()
            if line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    return parts[1]
                break
                
        return None


class Game:
    """Represents a single chess game"""
    
    def __init__(self, white, black, time_control=None, depth=None):
        self.white = white
        self.black = black
        self.time_control = time_control  # milliseconds per side
        self.depth = depth
        self.moves = []
        self.result = "*"
        self.termination = "unterminated"
        
    def play(self):
        """Play the game"""
        position = "startpos"
        move_count = 0
        max_moves = 200  # Maximum moves before declaring draw
        
        white_time = self.time_control if self.time_control else None
        black_time = self.time_control if self.time_control else None
        
        while move_count < max_moves:
            # White's turn
            current_engine = self.white
            current_time = white_time
            opponent_time = black_time
            
            if move_count % 2 == 1:
                # Black's turn
                current_engine = self.black
                current_time = black_time
                opponent_time = white_time
                
            # Get move
            try:
                start_time = time.time()
                
                if position == "startpos":
                    pos_str = "startpos"
                else:
                    pos_str = f"startpos moves {' '.join(self.moves)}"
                    
                move = current_engine.get_move(
                    pos_str,
                    wtime=white_time,
                    btime=black_time,
                    depth=self.depth
                )
                
                elapsed = int((time.time() - start_time) * 1000)
                
                if not move or move == "(none)" or move == "0000":
                    # No legal move - checkmate or stalemate
                    if move_count % 2 == 0:
                        self.result = "0-1"
                        self.termination = "Black wins"
                    else:
                        self.result = "1-0"
                        self.termination = "White wins"
                    break
                    
                self.moves.append(move)
                
                # Update time
                if current_time:
                    current_time = max(0, current_time - elapsed)
                    if current_time == 0:
                        if move_count % 2 == 0:
                            self.result = "0-1"
                            self.termination = "White loses on time"
                        else:
                            self.result = "1-0"
                            self.termination = "Black loses on time"
                        break
                        
                    if move_count % 2 == 0:
                        white_time = current_time
                    else:
                        black_time = current_time
                        
                move_count += 1
                
            except Exception as e:
                print(f"Error during game: {e}")
                self.result = "*"
                self.termination = f"Error: {e}"
                break
                
        if move_count >= max_moves:
            self.result = "1/2-1/2"
            self.termination = "Draw by move limit"
            
        return self.result
        
    def to_pgn(self):
        """Convert game to PGN format"""
        pgn = []
        pgn.append(f'[Event "Engine Match"]')
        pgn.append(f'[Date "{datetime.now().strftime("%Y.%m.%d")}"]')
        pgn.append(f'[White "{self.white.name}"]')
        pgn.append(f'[Black "{self.black.name}"]')
        pgn.append(f'[Result "{self.result}"]')
        
        if self.time_control:
            pgn.append(f'[TimeControl "{self.time_control/1000:.0f}"]')
        if self.depth:
            pgn.append(f'[Depth "{self.depth}"]')
            
        pgn.append(f'[Termination "{self.termination}"]')
        pgn.append("")
        
        # Format moves
        move_text = []
        for i, move in enumerate(self.moves):
            if i % 2 == 0:
                move_text.append(f"{i//2 + 1}. {move}")
            else:
                move_text.append(move)
                
        pgn.append(" ".join(move_text) + f" {self.result}")
        pgn.append("")
        
        return "\n".join(pgn)


class Tournament:
    """Manages a tournament between engines"""
    
    def __init__(self, engines, games_per_pair=10, time_control=None, depth=None):
        self.engines = engines
        self.games_per_pair = games_per_pair
        self.time_control = time_control
        self.depth = depth
        self.results = {engine.name: {"wins": 0, "losses": 0, "draws": 0} for engine in engines}
        self.games = []
        
    def run(self):
        """Run the tournament"""
        total_games = len(self.engines) * (len(self.engines) - 1) * self.games_per_pair
        game_num = 0
        
        print(f"Starting tournament with {len(self.engines)} engines")
        print(f"Games per pairing: {self.games_per_pair}")
        print(f"Total games: {total_games}")
        print("=" * 80)
        
        # Start all engines
        for engine in self.engines:
            print(f"Starting {engine.name}...")
            engine.start()
            
        # Play round-robin
        for i, engine1 in enumerate(self.engines):
            for j, engine2 in enumerate(self.engines):
                if i == j:
                    continue
                    
                # Play games_per_pair games (alternating colors)
                for game_idx in range(self.games_per_pair):
                    game_num += 1
                    
                    if game_idx % 2 == 0:
                        white, black = engine1, engine2
                    else:
                        white, black = engine2, engine1
                        
                    print(f"\nGame {game_num}/{total_games}: {white.name} vs {black.name}")
                    
                    game = Game(white, black, self.time_control, self.depth)
                    result = game.play()
                    
                    print(f"Result: {result} - {game.termination}")
                    print(f"Moves: {len(game.moves)}")
                    
                    # Update results
                    if result == "1-0":
                        self.results[white.name]["wins"] += 1
                        self.results[black.name]["losses"] += 1
                    elif result == "0-1":
                        self.results[black.name]["wins"] += 1
                        self.results[white.name]["losses"] += 1
                    elif result == "1/2-1/2":
                        self.results[white.name]["draws"] += 1
                        self.results[black.name]["draws"] += 1
                        
                    self.games.append(game)
                    
        # Stop all engines
        for engine in self.engines:
            engine.stop()
            
        return self.results
        
    def print_results(self):
        """Print tournament results"""
        print("\n" + "=" * 80)
        print("TOURNAMENT RESULTS")
        print("=" * 80)
        
        # Calculate scores
        scores = {}
        for name, result in self.results.items():
            score = result["wins"] + 0.5 * result["draws"]
            total = result["wins"] + result["losses"] + result["draws"]
            scores[name] = (score, total, result)
            
        # Sort by score
        sorted_engines = sorted(scores.items(), key=lambda x: x[1][0], reverse=True)
        
        print(f"{'Engine':<20} {'Score':<10} {'W-L-D':<15} {'Win%':<10}")
        print("-" * 80)
        
        for name, (score, total, result) in sorted_engines:
            win_pct = (result["wins"] / total * 100) if total > 0 else 0
            wld = f"{result['wins']}-{result['losses']}-{result['draws']}"
            print(f"{name:<20} {score:>4.1f}/{total:<4} {wld:<15} {win_pct:>5.1f}%")
            
        print("=" * 80)
        
    def save_pgn(self, filename):
        """Save all games to PGN file"""
        with open(filename, 'w') as f:
            for game in self.games:
                f.write(game.to_pgn())
                f.write("\n\n")
                
        print(f"\nGames saved to {filename}")
        
    def save_results(self, filename):
        """Save results to JSON file"""
        data = {
            "timestamp": datetime.now().isoformat(),
            "games_per_pair": self.games_per_pair,
            "time_control": self.time_control,
            "depth": self.depth,
            "results": self.results
        }
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
            
        print(f"Results saved to {filename}")


def main():
    parser = argparse.ArgumentParser(description="Play chess games between engines")
    parser.add_argument("engines", nargs="+", help="Paths to engine executables")
    parser.add_argument("-g", "--games", type=int, default=10, 
                       help="Games per pairing (default: 10)")
    parser.add_argument("-t", "--time", type=int, 
                       help="Time control in seconds per side")
    parser.add_argument("-d", "--depth", type=int,
                       help="Fixed search depth")
    parser.add_argument("-o", "--output", default="games.pgn",
                       help="Output PGN file (default: games.pgn)")
    parser.add_argument("-r", "--results", default="results.json",
                       help="Output results JSON file (default: results.json)")
    
    args = parser.parse_args()
    
    # Validate engines
    engines = []
    for engine_path in args.engines:
        if not os.path.exists(engine_path):
            print(f"Error: Engine not found: {engine_path}")
            sys.exit(1)
        engines.append(Engine(engine_path))
        
    if len(engines) < 2:
        print("Error: At least 2 engines required")
        sys.exit(1)
        
    # Convert time to milliseconds
    time_control = args.time * 1000 if args.time else None
    
    # Run tournament
    tournament = Tournament(engines, args.games, time_control, args.depth)
    tournament.run()
    tournament.print_results()
    tournament.save_pgn(args.output)
    tournament.save_results(args.results)


if __name__ == "__main__":
    main()
