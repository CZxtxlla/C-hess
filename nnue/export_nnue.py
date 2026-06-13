import torch
import torch.nn as nn
import numpy as np

# Re-define the model architecture to load the weights
class HalfKPNNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.transform_features = nn.EmbeddingBag(40960, 256, mode='sum')
        self.l1 = nn.Linear(512, 32)
        self.l2 = nn.Linear(32, 32)
        self.output = nn.Linear(32, 1)

# Load the trained weights
model = HalfKPNNUE()
model.load_state_dict(torch.load("nnue_checkpoint.pth", map_location='cpu', weights_only=True))

# Quantization
# multiply all floats by 255 and cast to 16-bit integers. 
# divide the final output by 255 to reverse in the C code
QA = 255.0 

def quantize_and_write(tensor, file, transpose=False):
    """Multiplies weights by QA, converts to int16, and writes to binary."""
    arr = tensor.detach().numpy()
    if transpose:
        # PyTorch stores linear weights as [out_features, in_features].
        # We transpose to [in_features, out_features] so C can read it cleanly.
        arr = arr.transpose()
        
    arr_quant = np.clip(np.round(arr * QA), -32768, 32767).astype(np.int16)
    file.write(arr_quant.tobytes())

print("Exporting weights to C binary format...")

with open("nnue_network.bin", "wb") as f:
    # --- Layer 0: The Feature Transformer (Accumulator) ---
    # Shape: (40960, 256) -> No transposition needed, EmbeddingBag is already [in, out]
    quantize_and_write(model.transform_features.weight, f, transpose=False)
    
    # Note: EmbeddingBag has no bias, but our C struct expects one. 
    # We write an array of 256 zeros so the C fread() doesn't fail.
    dummy_bias = np.zeros(256, dtype=np.int16)
    f.write(dummy_bias.tobytes())

    # --- Layer 1 ---
    # Shape: (32, 512) -> Transpose to (512, 32)
    quantize_and_write(model.l1.weight, f, transpose=True)
    quantize_and_write(model.l1.bias, f, transpose=False)

    # --- Layer 2 ---
    # Shape: (32, 32) -> Transpose to (32, 32)
    quantize_and_write(model.l2.weight, f, transpose=True)
    quantize_and_write(model.l2.bias, f, transpose=False)

    # --- Output Layer ---
    # Shape: (1, 32) -> Transpose to (32, 1)
    quantize_and_write(model.output.weight, f, transpose=True)
    quantize_and_write(model.output.bias, f, transpose=False)

print("Done! Saved as nnue_network.bin")