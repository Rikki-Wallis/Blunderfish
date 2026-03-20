import struct
import os
import glob
import time

import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
from torch.utils.data import random_split

torch.set_float32_matmul_precision('high')

RECORD_FORMAT = "=12Qii"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

def bitboards_to_tensor(bitboards):
    tensor = torch.zeros(768) 

    for color in [0, 1]:
        for piece in range(6):
            bb = bitboards[color*6+piece]

            for sq in range(64):
                if bb & (1 << sq) != 0:
                    tensor[(color*6+piece)*64+sq] = 1.0

    return tensor

class NNUEDataset(Dataset):
    def __init__(self, directory):
        self.samples = []
        for path in glob.glob(os.path.join(directory, "*.bin")):
            print(f"Loaded {path}")
            with open(path, "rb") as f:
                data = f.read()
            n = len(data) // RECORD_SIZE
            for i in range(n):
                offset = i * RECORD_SIZE
                fields = struct.unpack_from(RECORD_FORMAT, data, offset)
                self.samples.append(fields)

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        fields = self.samples[idx]

        bitboards = fields[:12]
        score = fields[12]
        outcome = fields[13] * 0.5 + 0.5

        x = bitboards_to_tensor(bitboards)
        wdl_score = torch.sigmoid(torch.tensor(score/400.0))
        target = 0.7 * wdl_score + 0.3 * outcome

        return x, target

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(768, 256),
            nn.ReLU(),
            nn.Linear(256, 32),
            nn.ReLU(),
            nn.Linear(32, 1),
            nn.Sigmoid()
        )

    def forward(self, x):
        return self.net(x).squeeze()
