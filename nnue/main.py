import torch
import torch.nn as nn
import torch.optim as optim


#data is in HalfKP, (own king position, own piece type, position of piece), (own king position, enemy piece type, position of piece), thus 40960 inputs (64 x 5 x 64) x 2


