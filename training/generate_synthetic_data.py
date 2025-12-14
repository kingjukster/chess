#!/usr/bin/env python3
"""
Generate synthetic training data without requiring PGN files.
Uses random positions and simple evaluation.
"""

import chess
import struct
import argparse
import random
from tqdm import tqdm

FEATURES_PER_KING = 64 * 5 * 2

def piece_to_index(piece_type, color):
    """Convert piece to feature index offset."""
    piece_map = {
        chess.PAWN: 0,
        chess.KNIGHT: 1,
        chess.BISHOP: 2,
        chess.ROOK: 3,
        chess.QUEEN: 4,
    }
    if piece_type not in piece_map:
        return None
    pt_idx = piece_map[piece_type]
    color_idx = 0 if color == chess.WHITE else 1
    return pt_idx * 2 + color_idx

def feature_index(king_sq, piece_sq, piece_type, piece_color):
    """Calculate HalfKP feature index."""
    piece_offset = piece_to_index(piece_type, piece_color)
    if piece_offset is None:
        return -1
    
    piece_sq_idx = piece_sq
    feature_offset = (piece_sq_idx * 5 + (piece_offset // 2)) * 2 + (piece_offset % 2)
    
    return king_sq * FEATURES_PER_KING + feature_offset

def extract_features(board):
    """Extract HalfKP features from position."""
    features = []
    
    white_king = board.king(chess.WHITE)
    black_king = board.king(chess.BLACK)
    
    if white_king is None or black_king is None:
        return features
    
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece is None or piece.piece_type == chess.KING:
            continue
        
        idx = feature_index(white_king, square, piece.piece_type, piece.color)
        if idx >= 0:
            features.append(idx)
        
        idx = feature_index(black_king, square, piece.piece_type, piece.color)
        if idx >= 0:
            features.append(idx)
    
    return features

def simple_evaluate(board):
    """Simple material-based evaluation."""
    piece_values = {
        chess.PAWN: 100,
        chess.KNIGHT: 320,
        chess.BISHOP: 330,
        chess.ROOK: 500,
        chess.QUEEN: 900,
    }
    
    material = 0
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece and piece.piece_type != chess.KING:
            value = piece_values.get(piece.piece_type, 0)
            material += value if piece.color == chess.WHITE else -value
    
    return material

def generate_positions(num_positions, output_file):
    """Generate random positions for training."""
    positions = []
    
    print(f"Generating {num_positions} random positions...")
    
    for _ in tqdm(range(num_positions)):
        # Create random position
        board = chess.Board()
        
        # Make some random moves (5-30 moves)
        num_moves = random.randint(5, 30)
        legal_moves = list(board.legal_moves)
        
        for _ in range(num_moves):
            if not legal_moves:
                break
            move = random.choice(legal_moves)
            board.push(move)
            legal_moves = list(board.legal_moves)
        
        # Extract features
        features = extract_features(board)
        if len(features) > 0:
            eval_score = simple_evaluate(board)
            positions.append((features, eval_score, board.turn))
    
    # Save to binary file
    print(f"Saving {len(positions)} positions to {output_file}...")
    with open(output_file, 'wb') as f:
        f.write(struct.pack('II', len(positions), FEATURES_PER_KING))
        
        for features, eval_score, stm in positions:
            f.write(struct.pack('iB', eval_score, 1 if stm else 0))
            f.write(struct.pack('H', len(features)))
            for feat in features:
                f.write(struct.pack('H', min(feat, 65535)))

def main():
    parser = argparse.ArgumentParser(description='Generate synthetic training data')
    parser.add_argument('--output', default='training_data.bin', help='Output file')
    parser.add_argument('--num-positions', type=int, default=100000, help='Number of positions to generate')
    
    args = parser.parse_args()
    
    generate_positions(args.num_positions, args.output)
    print(f"Done! Generated {args.num_positions} positions in {args.output}")

if __name__ == '__main__':
    main()

