import torch
import torch.nn as nn
import math
import chess

# 1. Define the exact same model architecture
"""
class HalfKPNNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.transform_features = nn.EmbeddingBag(40960, 256, mode='sum')
        self.l1 = nn.Linear(512, 32)
        self.l2 = nn.Linear(32, 32)
        self.output = nn.Linear(32, 1)

    def forward(self, w_coords, b_coords):
        w_accum = torch.clamp(self.transform_features(w_coords[0], w_coords[1]), 0.0, 1.0)
        b_accum = torch.clamp(self.transform_features(b_coords[0], b_coords[1]), 0.0, 1.0)
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

# 2. Helper functions for feature extraction
def piece_to_int(piece, perspective_is_white):
    """Maps pieces based on perspective (Friendly=0-4, Enemy=5-9)"""
    if piece is None: return -1
    
    is_white_piece = piece.color == chess.WHITE
    is_friendly = (perspective_is_white and is_white_piece) or (not perspective_is_white and not is_white_piece)
    
    base = piece.piece_type - 1 # 0 to 4 (P, N, B, R, Q)
    return base if is_friendly else base + 5

def extract_indices(board):
    w_indices, b_indices = [], []
    
    w_king_sq = board.king(chess.WHITE)
    b_king_sq = board.king(chess.BLACK)
    if w_king_sq is None or b_king_sq is None: return [], []

    for sq, piece in board.piece_map().items():
        if piece.piece_type == chess.KING: continue
        
        # White Perspective
        w_p_type = piece_to_int(piece, perspective_is_white=True)
        w_indices.append(w_king_sq * 640 + w_p_type * 64 + sq)
        
        # Black Perspective (Flip squares!)
        b_p_type = piece_to_int(piece, perspective_is_white=False)
        # FIXED: Use b_king_sq instead of sq for the first multiplier
        b_indices.append((b_king_sq ^ 56) * 640 + b_p_type * 64 + (sq ^ 56)) 

    return w_indices, b_indices


# 3. Load Model
device = torch.device("cpu") # CPU is fine for single inferences
model = HalfKPNNUE().to(device)
model.load_state_dict(torch.load("nnue_checkpoint.pth", map_location=device, weights_only=True))
model.eval() # Set to evaluation mode

def evaluate_fen(fen):
    board = chess.Board(fen)
    w_idx, b_idx = extract_indices(board)
    
    # Convert to Tensors
    w_col = torch.tensor(w_idx, dtype=torch.long)
    b_col = torch.tensor(b_idx, dtype=torch.long)
    offsets = torch.tensor([0], dtype=torch.long) # Batch size is 1, so offset is just 0
    
    # Run Inference
    with torch.no_grad():
        w_coords = (w_col, offsets)
        b_coords = (b_col, offsets)
        win_prob = model(w_coords, b_coords).item()
    
    # Reverse the Sigmoid to get Centipawns
    # p = 1 / (1 + exp(-cp/400))  --->  cp = -400 * ln(1/p - 1)
    win_prob = max(0.001, min(0.999, win_prob)) # Prevent math errors on absolute 0 or 1
    centipawns = -400.0 * math.log((1.0 / win_prob) - 1.0)
    
    print(f"FEN: {fen}")
    print(f"  -> Win Probability: {win_prob * 100:.2f}%")
    print(f"  -> Eval: {centipawns / 100.0:+.2f} Pawns\n")

# --- THE SANITY CHECKS ---
print("--- Model Sanity Checks ---")
# 1. Start Position (Should be slightly positive for White, e.g., +0.20 to +0.40)
evaluate_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

# 2. White is completely winning (Up a Queen)
evaluate_fen("8/8/8/8/8/8/4Q3/4K2k w - - 0 1")

# 3. Black is completely winning (Up a Queen)
evaluate_fen("4k3/4q3/8/8/8/8/8/4K3 w - - 0 1")

# 4. A totally drawn King + Pawn endgame
evaluate_fen("8/8/8/4k3/8/4K3/4P3/8 w - - 0 1")