import torch
import struct
import chess_dataloader

# 1. Create a dummy binary dataset to test with
print("Generating dummy data.bin...")
with open("data.bin", "wb") as f:
    # Let's create 2 dummy chess positions
    for _ in range(2):
        board = [0] * 64
        board[4] = 6   # White King on E1
        board[60] = 12 # Black King on E8
        board[12] = 1  # White Pawn
        board[52] = 7  # Black Pawn
        
        # Write 64 bytes for the board, then 4 bytes for the float target (1.0)
        f.write(bytes(board))
        f.write(struct.pack("f", 1.0))

# 2. Load the data using your new C++ Extension!
print("\nInitializing FastDataLoader...")
BATCH_SIZE = 2
loader = chess_dataloader.FastDataLoader("data.bin", BATCH_SIZE)

# 3. Fetch a batch
batch_data = loader.next_batch()

if len(batch_data) == 0:
    print("Failed to read batch (EOF).")
else:
    w_row, w_col, b_row, b_col, targets = batch_data
    print("\n--- Success! Loaded Tensors ---")
    print(f"Targets shape: {targets.shape}\nValues: {targets}\n")
    
    print("White Perspective Non-Zeros:")
    print(f"Rows: {w_row}")
    print(f"Cols (HalfKP Indices): {w_col}\n")
    
    print("Black Perspective Non-Zeros:")
    print(f"Rows: {b_row}")
    print(f"Cols (HalfKP Indices): {b_col}")