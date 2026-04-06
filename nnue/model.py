import struct
import mmap
import os
import numpy as np

import torch
import torch.nn as nn
from torch.utils.data import Dataset
import torch.nn.functional as F

torch.set_float32_matmul_precision('high')

RECORD_FORMAT = "=12Qibb"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

def bb_to_squares(bb: int) -> np.ndarray:
    arr = np.array([bb], dtype=np.uint64).view(np.uint8)  # 8 bytes
    bits = np.unpackbits(arr, bitorder='little')           # 64 bits, LSB first
    return np.where(bits)[0]      

def bitboards_to_indices(bitboards: list[int], persp:int):
    indices = []

    for piece_side in range(2):
        for piece in range(6):
            bb = bitboards[piece_side*6+piece]

            if not bb: continue

            sqs = bb_to_squares(bb);

            rel_sq = sqs ^ 56 if persp == 1 else sqs
            rel_side = (1-piece_side) if persp == 1 else piece_side

            indices.append(rel_side * 6 * 64 + piece * 64 + rel_sq)

    return np.concatenate(indices) if indices else np.array([], dtype=np.int32)

class NNUEDataset(Dataset):
    def __init__(self, path, start=0, end=None):
        self.path = path
        self.mm = None
        total = os.path.getsize(path) // RECORD_SIZE 
        self.start = start
        self.end = end if end is not None else total

    def _ensure_open(self):
        if self.mm is None:
            self.f = open(self.path, "rb")
            self.mm = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)

    def __del__(self):
        if self.mm is not None:
            self.mm.close()
            self.f.close()

    def __len__(self):
        return self.end - self.start

    def __getitem__(self, idx):
        self._ensure_open()
        offset = (idx+self.start) * RECORD_SIZE
        return bytes(self.mm[offset : offset + RECORD_SIZE])

        white, black, wdl_score, outcome = ext.parse_record(raw);
        target = (1-CUR_BLEND) * wdl_score + CUR_BLEND * outcome
        return white, black, torch.tensor(target, dtype=torch.float32)

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()

        self.l1 = nn.Linear(2*6*64, 64)
        self.l2 = nn.Linear(128, 32)
        self.l3 = nn.Linear(32, 1)

    @torch.compiler.disable
    def feed_l1(self, features, indices):
        f = self.l1.weight[:, features].T
        a1 = torch.zeros(indices.max()+1, 64, device=features.device)
        a1.index_add_(0, indices, f)
        return a1 + self.l1.bias

    def forward(self, white_features, white_indices, black_features, black_indices):
        a1w = self.feed_l1(white_features, white_indices)
        a1b = self.feed_l1(black_features, black_indices)

        a1 = torch.concat([a1w, a1b], dim=1)
        a1 = F.hardtanh(a1, 0, 1)

        a2 = F.hardtanh(self.l2(a1), 0, 1)
        a3 = F.sigmoid(self.l3(a2))

        return a3.squeeze(-1)
