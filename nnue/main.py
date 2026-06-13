import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
import chess_dataloader


#data is in HalfKP, (own king position, own piece type, position of piece), (own king position, enemy piece type, position of piece), thus 40960 inputs (64 x 5 x 64) x 2

"""
class HalfKPNNUE(nn.Module):
    def __init__(self):
        super().__init__()
        # For sparse inputs, EmbeddingBag is vastly faster than nn.Linear.
        # It takes our indices and automatically sums the weights of the active pieces.
        self.transform_features = nn.EmbeddingBag(40960, 256, mode='sum')
        
        self.l1 = nn.Linear(512, 32)
        self.l2 = nn.Linear(32, 32)
        self.output = nn.Linear(32, 1)

    def forward(self, w_coords, b_coords):
        # We clamp the output of the first layer (Clipped ReLU)
        # Passed as: self.transform_features(indices, offsets)
        w_accum = torch.clamp(self.transform_features(w_coords[0], w_coords[1]), 0.0, 1.0)
        b_accum = torch.clamp(self.transform_features(b_coords[0], b_coords[1]), 0.0, 1.0)

        # Concatenate White and Black accumulators (256 + 256 = 512)
        x = torch.cat([w_accum, b_accum], dim=1)
        
        x = torch.clamp(self.l1(x), 0.0, 1.0)
        x = torch.clamp(self.l2(x), 0.0, 1.0)
        return self.output(x)
"""

class HalfKPNNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.transform_features = nn.EmbeddingBag(40960, 256, mode='sum')
        self.l1 = nn.Linear(512, 32)
        self.l2 = nn.Linear(32, 32)
        self.output = nn.Linear(32, 1)

        # --- THE FIX: Custom Weight Initialization ---
        # Initialize the embedding with very small values so the sum of 30 pieces
        # stays mostly between 0.0 and 1.0, keeping the clamp gradients alive!
        nn.init.uniform_(self.transform_features.weight, -0.01, 0.01)
        
        # Standard Kaiming initialization for the hidden layers
        nn.init.kaiming_uniform_(self.l1.weight, nonlinearity='relu')
        nn.init.kaiming_uniform_(self.l2.weight, nonlinearity='relu')

    def forward(self, w_coords, b_coords):
        w_accum = torch.clamp(self.transform_features(w_coords[0], w_coords[1]), 0.0, 1.0)
        b_accum = torch.clamp(self.transform_features(b_coords[0], b_coords[1]), 0.0, 1.0)

        x = torch.cat([w_accum, b_accum], dim=1)
        
        x = torch.clamp(self.l1(x), 0.0, 1.0)
        x = torch.clamp(self.l2(x), 0.0, 1.0)
        
        # --- THE FIX: Apply Sigmoid ---
        # Squeeze the final output into a 0.0 to 1.0 probability
        return torch.sigmoid(self.output(x))

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Training on: {device}")

model = HalfKPNNUE().to(device)
optimizer = optim.Adam(model.parameters(), lr=1e-3)
criterion = nn.MSELoss()

BATCH_SIZE = 32768 # You can easily do 8k-32k batches now because of your C loader!
EPOCHS = 10

# 3. Training Loop
for epoch in range(EPOCHS):
    # Re-initialize the loader at the start of every epoch
    loader = chess_dataloader.FastDataLoader("data.bin", BATCH_SIZE)
    
    batch_idx = 0
    total_loss = 0.0
    
    while True:
        batch_data = loader.next_batch()
        if len(batch_data) == 0:
            break # Reached the end of data.bin
            
        w_row, w_col, b_row, b_col, targets = batch_data
        
        # Move raw data to GPU
        w_row, w_col = w_row.to(device), w_col.to(device)
        b_row, b_col = b_row.to(device), b_col.to(device)
        targets = targets.to(device).unsqueeze(1) # Shape [Batch, 1]
        
        # --- THE FIX: Convert COO row indices to EmbeddingBag offsets ---
        # Get the actual batch size for this step (handles the final partial batch safely)
        current_batch_size = targets.size(0) 
        
        # 1. Count how many pieces belong to each board in the batch
        w_counts = torch.bincount(w_row, minlength=current_batch_size)
        b_counts = torch.bincount(b_row, minlength=current_batch_size)
        
        # 2. Cumulative sum minus the counts gives us the starting offset for each bag!
        w_offsets = w_counts.cumsum(0) - w_counts
        b_offsets = b_counts.cumsum(0) - b_counts
        
        # Package them up: (Indices, Offsets)
        w_coords = (w_col, w_offsets)
        b_coords = (b_col, b_offsets)
        # ----------------------------------------------------------------
        
        # Forward Pass
        optimizer.zero_grad()
        predictions = model(w_coords, b_coords)
        
        # Loss and Backprop
        loss = criterion(predictions, targets)
        loss.backward()
        optimizer.step()
        
        total_loss += loss.item()
        batch_idx += 1
        
        if batch_idx % 100 == 0:
            print(f"Epoch {epoch+1} | Batch {batch_idx} | Loss: {loss.item():.4f}")

    print(f"--- Epoch {epoch+1} Complete | Avg Loss: {total_loss/batch_idx:.4f} ---")

# Save the PyTorch weights so you can export them to your C engine later
torch.save(model.state_dict(), "nnue_checkpoint.pth")
print("Training complete! Model saved.")