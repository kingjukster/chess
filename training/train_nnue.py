#!/usr/bin/env python3
"""
Train NNUE network on prepared training data.
Uses PyTorch for training.
"""

import torch
import torch.nn as nn
import torch.optim as optim
import struct
import argparse
import numpy as np
from torch.utils.data import Dataset, DataLoader
from tqdm import tqdm

# Network architecture (matches engine)
FEATURES_PER_KING = 64 * 5 * 2  # 640 features per king
HIDDEN_SIZE = 256
OUTPUT_SIZE = 1

class NnueDataset(Dataset):
    """Dataset for NNUE training."""
    
    def __init__(self, data_file):
        self.positions = []
        self.evaluations = []
        self.sides = []
        
        print(f"Loading data from {data_file}...")
        with open(data_file, 'rb') as f:
            num_positions, feature_size = struct.unpack('II', f.read(8))
            
            for _ in tqdm(range(num_positions)):
                # Read evaluation and side
                eval_score, stm = struct.unpack('iB', f.read(5))
                
                # Read features
                num_features = struct.unpack('H', f.read(2))[0]
                features = struct.unpack(f'{num_features}H', f.read(2 * num_features))
                
                # Create sparse feature vector
                feature_vector = torch.zeros(FEATURES_PER_KING, dtype=torch.float32)
                for feat_idx in features:
                    if feat_idx < FEATURES_PER_KING:
                        feature_vector[feat_idx] = 1.0
                
                self.positions.append(feature_vector)
                self.evaluations.append(eval_score / 100.0)  # Convert centipawns to pawns
                self.sides.append(stm)
        
        print(f"Loaded {len(self.positions)} positions")
    
    def __len__(self):
        return len(self.positions)
    
    def __getitem__(self, idx):
        return self.positions[idx], self.evaluations[idx], self.sides[idx]

class NnueNetwork(nn.Module):
    """NNUE network architecture."""
    
    def __init__(self, input_size=FEATURES_PER_KING, hidden_size=HIDDEN_SIZE, output_size=OUTPUT_SIZE):
        super(NnueNetwork, self).__init__()
        
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.output_size = output_size
        
        # Input to hidden layer
        self.input_layer = nn.Linear(input_size, hidden_size, bias=True)
        
        # Hidden to output layer
        self.output_layer = nn.Linear(hidden_size, output_size, bias=True)
        
        # Initialize weights
        nn.init.xavier_uniform_(self.input_layer.weight)
        nn.init.xavier_uniform_(self.output_layer.weight)
        nn.init.zeros_(self.input_layer.bias)
        nn.init.zeros_(self.output_layer.bias)
    
    def forward(self, x):
        # Input layer
        hidden = self.input_layer(x)
        
        # Clipped ReLU activation (clamp between 0 and 1, scaled)
        hidden = torch.clamp(hidden, 0, 1.0)
        
        # Output layer
        output = self.output_layer(hidden)
        
        return output

def train_epoch(model, dataloader, criterion, optimizer, device):
    """Train for one epoch."""
    model.train()
    total_loss = 0.0
    num_batches = 0
    
    for features, evaluations, sides in tqdm(dataloader, desc="Training"):
        features = features.to(device)
        evaluations = evaluations.to(device).float()
        sides = sides.to(device)
        
        # Forward pass
        outputs = model(features).squeeze()
        
        # Adjust for side to move (flip evaluation if black to move)
        outputs = outputs * (1 - 2 * sides.float())
        
        # Calculate loss
        loss = criterion(outputs, evaluations)
        
        # Backward pass
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        
        total_loss += loss.item()
        num_batches += 1
    
    return total_loss / num_batches

def validate(model, dataloader, criterion, device):
    """Validate model."""
    model.eval()
    total_loss = 0.0
    num_batches = 0
    
    with torch.no_grad():
        for features, evaluations, sides in dataloader:
            features = features.to(device)
            evaluations = evaluations.to(device).float()
            sides = sides.to(device)
            
            outputs = model(features).squeeze()
            outputs = outputs * (1 - 2 * sides.float())
            
            loss = criterion(outputs, evaluations)
            total_loss += loss.item()
            num_batches += 1
    
    return total_loss / num_batches if num_batches > 0 else 0.0

def main():
    parser = argparse.ArgumentParser(description='Train NNUE network')
    parser.add_argument('--data', required=True, help='Training data file')
    parser.add_argument('--epochs', type=int, default=10, help='Number of epochs')
    parser.add_argument('--batch-size', type=int, default=1024, help='Batch size')
    parser.add_argument('--learning-rate', type=float, default=0.001, help='Learning rate')
    parser.add_argument('--output', default='nnue_model.pth', help='Output model file')
    parser.add_argument('--device', default='cuda' if torch.cuda.is_available() else 'cpu', help='Device to use')
    
    args = parser.parse_args()
    
    device = torch.device(args.device)
    print(f"Using device: {device}")
    
    # Load dataset
    dataset = NnueDataset(args.data)
    
    # Split into train/validation (90/10)
    train_size = int(0.9 * len(dataset))
    val_size = len(dataset) - train_size
    train_dataset, val_dataset = torch.utils.data.random_split(dataset, [train_size, val_size])
    
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False)
    
    # Create model
    model = NnueNetwork().to(device)
    print(f"Model parameters: {sum(p.numel() for p in model.parameters())}")
    
    # Loss and optimizer
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=args.learning_rate)
    scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=5, gamma=0.5)
    
    # Training loop
    best_val_loss = float('inf')
    
    for epoch in range(args.epochs):
        print(f"\nEpoch {epoch + 1}/{args.epochs}")
        
        train_loss = train_epoch(model, train_loader, criterion, optimizer, device)
        val_loss = validate(model, val_loader, criterion, device)
        
        print(f"Train Loss: {train_loss:.4f}, Val Loss: {val_loss:.4f}")
        
        scheduler.step()
        
        # Save best model
        if val_loss < best_val_loss:
            best_val_loss = val_loss
            torch.save(model.state_dict(), args.output)
            print(f"Saved best model to {args.output}")
    
    print(f"\nTraining complete! Best validation loss: {best_val_loss:.4f}")
    print(f"Model saved to {args.output}")

if __name__ == '__main__':
    main()

