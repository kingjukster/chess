#!/usr/bin/env python3
"""
Polyglot Opening Book Generator

This tool generates Polyglot format opening books from PGN files.
It can filter games by rating, result, and opening, and weight moves
by frequency and success rate.

Usage:
    python make_book.py input.pgn output.bin [options]

Options:
    --min-rating RATING    Minimum player rating (default: 2000)
    --max-ply PLY          Maximum ply depth to include (default: 40)
    --min-games GAMES      Minimum games for a position (default: 3)
    --result RESULT        Filter by result: 1-0, 0-1, 1/2-1/2, or * for all (default: *)
    --opening OPENING      Filter by opening name (substring match)
    --weight-by-result     Weight moves by win/draw/loss statistics
    --uniform-weights      Use uniform weights (ignore frequency)
"""

import sys
import struct
import argparse
from collections import defaultdict
import chess
import chess.pgn

# Polyglot zobrist keys (standard keys used by all Polyglot books)
POLYGLOT_RANDOM = [
    0x9D39247E33776D41, 0x2AF7398005AAA5C7, 0x44DB015024623547, 0x9C15F73E62A76AE2,
    # ... (full 781 keys would be here, using subset for brevity)
    # In production, all 781 standard Polyglot keys should be included
]

def polyglot_hash(board):
    """Calculate Polyglot hash for a chess position."""
    h = 0
    
    # Pieces (64 squares * 12 piece types)
    piece_map = {
        chess.PAWN: 0, chess.KNIGHT: 1, chess.BISHOP: 2,
        chess.ROOK: 3, chess.QUEEN: 4, chess.KING: 5
    }
    
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            piece_idx = piece_map[piece.piece_type]
            if piece.color == chess.BLACK:
                piece_idx += 6
            
            # Polyglot uses flipped square ordering
            poly_square = square ^ 56
            h ^= POLYGLOT_RANDOM[64 * piece_idx + poly_square]
    
    # Castling rights (indices 768-771)
    if board.has_kingside_castling_rights(chess.WHITE):
        h ^= POLYGLOT_RANDOM[768]
    if board.has_queenside_castling_rights(chess.WHITE):
        h ^= POLYGLOT_RANDOM[769]
    if board.has_kingside_castling_rights(chess.BLACK):
        h ^= POLYGLOT_RANDOM[770]
    if board.has_queenside_castling_rights(chess.BLACK):
        h ^= POLYGLOT_RANDOM[771]
    
    # En passant file (indices 772-779)
    if board.ep_square:
        h ^= POLYGLOT_RANDOM[772 + chess.square_file(board.ep_square)]
    
    # Side to move (index 780)
    if board.turn == chess.WHITE:
        h ^= POLYGLOT_RANDOM[780]
    
    return h

def move_to_polyglot(move):
    """Convert chess.Move to Polyglot move format."""
    from_sq = move.from_square ^ 56  # Flip vertically
    to_sq = move.to_square ^ 56
    
    promo = 0
    if move.promotion:
        promo_map = {
            chess.KNIGHT: 1, chess.BISHOP: 2,
            chess.ROOK: 3, chess.QUEEN: 4
        }
        promo = promo_map.get(move.promotion, 0)
    
    return (promo << 12) | (from_sq << 6) | to_sq

class BookEntry:
    """Represents a single book entry (position + move)."""
    def __init__(self, key, move):
        self.key = key
        self.move = move
        self.weight = 0
        self.wins = 0
        self.draws = 0
        self.losses = 0
        self.count = 0
    
    def add_game(self, result):
        """Add a game result to this entry."""
        self.count += 1
        if result == "1-0":
            self.wins += 1
        elif result == "0-1":
            self.losses += 1
        elif result == "1/2-1/2":
            self.draws += 1
    
    def calculate_weight(self, weight_by_result=False, uniform=False):
        """Calculate the weight for this entry."""
        if uniform:
            self.weight = 1
        elif weight_by_result and self.count > 0:
            # Weight by success rate: wins=1.0, draws=0.5, losses=0.0
            success_rate = (self.wins + 0.5 * self.draws) / self.count
            self.weight = int(success_rate * self.count * 100)
        else:
            # Weight by frequency
            self.weight = self.count
        
        # Ensure minimum weight
        self.weight = max(1, min(65535, self.weight))
    
    def to_bytes(self):
        """Convert entry to Polyglot binary format (16 bytes)."""
        return struct.pack('>QHHH',
                          self.key,      # 8 bytes: position hash
                          self.move,     # 2 bytes: move
                          self.weight,   # 2 bytes: weight
                          0)             # 4 bytes: learn (unused)

class BookBuilder:
    """Builds a Polyglot opening book from PGN games."""
    
    def __init__(self, min_rating=2000, max_ply=40, min_games=3):
        self.min_rating = min_rating
        self.max_ply = max_ply
        self.min_games = min_games
        self.entries = defaultdict(lambda: defaultdict(lambda: BookEntry(0, 0)))
        self.games_processed = 0
        self.positions_added = 0
    
    def add_game(self, game, result_filter="*", opening_filter=None):
        """Add a game to the book."""
        # Check filters
        headers = game.headers
        
        # Rating filter
        white_elo = int(headers.get("WhiteElo", 0))
        black_elo = int(headers.get("BlackElo", 0))
        if white_elo < self.min_rating or black_elo < self.min_rating:
            return False
        
        # Result filter
        result = headers.get("Result", "*")
        if result_filter != "*" and result != result_filter:
            return False
        
        # Opening filter
        if opening_filter:
            opening = headers.get("Opening", "")
            if opening_filter.lower() not in opening.lower():
                return False
        
        # Process moves
        board = game.board()
        ply = 0
        
        for move in game.mainline_moves():
            if ply >= self.max_ply:
                break
            
            # Get position hash and move
            key = polyglot_hash(board)
            poly_move = move_to_polyglot(move)
            
            # Add to book
            if key not in self.entries or poly_move not in self.entries[key]:
                self.entries[key][poly_move] = BookEntry(key, poly_move)
                self.positions_added += 1
            
            self.entries[key][poly_move].add_game(result)
            
            # Make move
            board.push(move)
            ply += 1
        
        self.games_processed += 1
        return True
    
    def build(self, pgn_file, result_filter="*", opening_filter=None):
        """Build book from PGN file."""
        print(f"Building book from {pgn_file}...")
        print(f"Filters: min_rating={self.min_rating}, max_ply={self.max_ply}, "
              f"result={result_filter}, opening={opening_filter or 'any'}")
        
        with open(pgn_file, 'r', encoding='utf-8', errors='ignore') as f:
            while True:
                game = chess.pgn.read_game(f)
                if game is None:
                    break
                
                self.add_game(game, result_filter, opening_filter)
                
                if self.games_processed % 100 == 0:
                    print(f"Processed {self.games_processed} games, "
                          f"{self.positions_added} positions...", end='\r')
        
        print(f"\nProcessed {self.games_processed} games, "
              f"{self.positions_added} positions")
    
    def prune(self):
        """Remove entries with too few games."""
        print(f"Pruning entries with < {self.min_games} games...")
        
        pruned = 0
        for key in list(self.entries.keys()):
            for move in list(self.entries[key].keys()):
                if self.entries[key][move].count < self.min_games:
                    del self.entries[key][move]
                    pruned += 1
            
            if not self.entries[key]:
                del self.entries[key]
        
        print(f"Pruned {pruned} entries")
    
    def calculate_weights(self, weight_by_result=False, uniform=False):
        """Calculate weights for all entries."""
        print("Calculating weights...")
        
        for key in self.entries:
            for move in self.entries[key]:
                self.entries[key][move].calculate_weight(weight_by_result, uniform)
    
    def save(self, output_file):
        """Save book to Polyglot binary format."""
        print(f"Saving book to {output_file}...")
        
        # Flatten entries and sort by key
        all_entries = []
        for key in self.entries:
            for move in self.entries[key]:
                all_entries.append(self.entries[key][move])
        
        all_entries.sort(key=lambda e: e.key)
        
        # Write to file
        with open(output_file, 'wb') as f:
            for entry in all_entries:
                f.write(entry.to_bytes())
        
        print(f"Saved {len(all_entries)} entries to {output_file}")
        
        # Print statistics
        total_weight = sum(e.weight for e in all_entries)
        avg_weight = total_weight / len(all_entries) if all_entries else 0
        print(f"Total weight: {total_weight}, Average weight: {avg_weight:.2f}")

def main():
    parser = argparse.ArgumentParser(
        description='Generate Polyglot opening book from PGN file')
    parser.add_argument('input', help='Input PGN file')
    parser.add_argument('output', help='Output book file (.bin)')
    parser.add_argument('--min-rating', type=int, default=2000,
                       help='Minimum player rating (default: 2000)')
    parser.add_argument('--max-ply', type=int, default=40,
                       help='Maximum ply depth (default: 40)')
    parser.add_argument('--min-games', type=int, default=3,
                       help='Minimum games for position (default: 3)')
    parser.add_argument('--result', default='*',
                       help='Filter by result: 1-0, 0-1, 1/2-1/2, * (default: *)')
    parser.add_argument('--opening', default=None,
                       help='Filter by opening name (substring match)')
    parser.add_argument('--weight-by-result', action='store_true',
                       help='Weight moves by win/draw/loss statistics')
    parser.add_argument('--uniform-weights', action='store_true',
                       help='Use uniform weights (ignore frequency)')
    
    args = parser.parse_args()
    
    # Check if python-chess is installed
    try:
        import chess
        import chess.pgn
    except ImportError:
        print("Error: python-chess library not found")
        print("Install it with: pip install python-chess")
        sys.exit(1)
    
    # Build book
    builder = BookBuilder(args.min_rating, args.max_ply, args.min_games)
    builder.build(args.input, args.result, args.opening)
    builder.prune()
    builder.calculate_weights(args.weight_by_result, args.uniform_weights)
    builder.save(args.output)
    
    print("\nDone!")

if __name__ == '__main__':
    main()
