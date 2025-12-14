#!/usr/bin/env python3
"""
Prepare training data from PGN files or position databases.
Extracts positions and evaluations, converts to HalfKP features.
"""

import chess
import chess.pgn
import struct
import argparse
import numpy as np
from tqdm import tqdm

# HalfKP feature extraction (matches engine's feature_index)
KING_BUCKETS = 64
PIECE_SQUARES = 64
PIECE_TYPES = 5  # PAWN, KNIGHT, BISHOP, ROOK, QUEEN
FEATURES_PER_KING = PIECE_SQUARES * PIECE_TYPES * 2  # 2 colors

def piece_to_index(piece_type, color):
    """Convert piece to feature index offset."""
    if piece_type == chess.PAWN:
        pt_idx = 0
    elif piece_type == chess.KNIGHT:
        pt_idx = 1
    elif piece_type == chess.BISHOP:
        pt_idx = 2
    elif piece_type == chess.ROOK:
        pt_idx = 3
    elif piece_type == chess.QUEEN:
        pt_idx = 4
    else:
        return None  # KING or NO_PIECE
    
    color_idx = 0 if color == chess.WHITE else 1
    return pt_idx * 2 + color_idx

def square_to_index(square):
    """Convert chess square (0-63) to index."""
    return square

def feature_index(king_sq, piece_sq, piece_type, piece_color):
    """Calculate HalfKP feature index (matches C++ implementation)."""
    piece_offset = piece_to_index(piece_type, piece_color)
    if piece_offset is None:
        return -1
    
    piece_sq_idx = square_to_index(piece_sq)
    feature_offset = (piece_sq_idx * PIECE_TYPES + (piece_offset // 2)) * 2 + (piece_offset % 2)
    
    return king_sq * FEATURES_PER_KING + feature_offset

def extract_features(board):
    """Extract HalfKP features from a chess position."""
    features = []
    
    # Get king squares
    white_king = board.king(chess.WHITE)
    black_king = board.king(chess.BLACK)
    
    if white_king is None or black_king is None:
        return features
    
    # Extract features from white's perspective
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece is None or piece.piece_type == chess.KING:
            continue
        
        # Feature relative to white king
        idx = feature_index(white_king, square, piece.piece_type, piece.color)
        if idx >= 0:
            features.append(idx)
        
        # Feature relative to black king
        idx = feature_index(black_king, square, piece.piece_type, piece.color)
        if idx >= 0:
            features.append(idx)
    
    return features

def evaluate_position(board, depth=8):
    """Evaluate position using stockfish (if available) or simple heuristic."""
    try:
        import stockfish
        engine = stockfish.Stockfish()
        engine.set_fen_position(board.fen())
        eval_score = engine.get_evaluation()
        if eval_score['type'] == 'cp':
            return eval_score['value']
        elif eval_score['type'] == 'mate':
            # Convert mate to centipawns (approximate)
            return 30000 if eval_score['value'] > 0 else -30000
    except:
        pass
    
    # Fallback: simple material evaluation
    material = 0
    piece_values = {
        chess.PAWN: 100,
        chess.KNIGHT: 320,
        chess.BISHOP: 330,
        chess.ROOK: 500,
        chess.QUEEN: 900,
        chess.KING: 20000
    }
    
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            value = piece_values[piece.piece_type]
            material += value if piece.color == chess.WHITE else -value
    
    return material

def process_pgn_file(pgn_file, output_file, max_positions=1000000):
    """Process PGN file and extract training positions."""
    positions = []
    
    print(f"Processing {pgn_file}...")
    
    with open(pgn_file, 'r') as f:
        game_count = 0
        position_count = 0
        
        while True:
            game = chess.pgn.read_game(f)
            if game is None:
                break
            
            game_count += 1
            board = game.board()
            
            # Sample positions from this game
            move_count = 0
            for move in game.mainline_moves():
                board.push(move)
                move_count += 1
                
                # Sample every 5th move, skip first 5 moves
                if move_count > 5 and move_count % 5 == 0:
                    # Extract features
                    features = extract_features(board)
                    if len(features) > 0:
                        # Evaluate position
                        eval_score = evaluate_position(board)
                        
                        # Store: (features, evaluation, side_to_move)
                        positions.append((features, eval_score, board.turn))
                        position_count += 1
                        
                        if position_count >= max_positions:
                            break
            
            if position_count >= max_positions:
                break
            
            if game_count % 100 == 0:
                print(f"Processed {game_count} games, {position_count} positions")
        
        print(f"Total: {game_count} games, {position_count} positions")
    
    # Save to binary file
    print(f"Saving {len(positions)} positions to {output_file}...")
    with open(output_file, 'wb') as f:
        # Write header: number of positions, feature size
        f.write(struct.pack('II', len(positions), FEATURES_PER_KING))
        
        for features, eval_score, stm in tqdm(positions):
            # Write: evaluation (int32), side_to_move (uint8), num_features (uint16), features (uint16[])
            f.write(struct.pack('iB', eval_score, 1 if stm else 0))
            f.write(struct.pack('H', len(features)))
            for feat in features:
                f.write(struct.pack('H', min(feat, 65535)))  # Clamp to uint16

def main():
    parser = argparse.ArgumentParser(description='Prepare NNUE training data from PGN files')
    parser.add_argument('--input', required=True, help='Input PGN file')
    parser.add_argument('--output', required=True, help='Output binary file')
    parser.add_argument('--max-positions', type=int, default=1000000, help='Maximum positions to extract')
    
    args = parser.parse_args()
    
    process_pgn_file(args.input, args.output, args.max_positions)
    print(f"Done! Saved training data to {args.output}")

if __name__ == '__main__':
    main()

