import struct
import mmap
import os

import torch
import torch.nn as nn
from torch.utils.data import Dataset
import torch.nn.functional as F

torch.set_float32_matmul_precision('high')

RECORD_FORMAT = "=12Qii"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

def lsb(x: int):
    assert x != 0
    return (x & -x).bit_length() - 1

def flip_sq(sq):
    return sq ^ 56

def bitboards_to_indices(bitboards: list[int], king_side:int):
    WHITE = 0
    BLACK = 1

    king_bb_idx = [
        5, 11
    ]

    king_sq = lsb(bitboards[king_bb_idx[king_side]])
    king_sq = flip_sq(king_sq) if king_side == BLACK else king_sq

    indices = []

    for piece_side in [WHITE, BLACK]:
        rel_side = 1-piece_side if king_side == BLACK else piece_side

        for piece in range(5):
            bb = bitboards[piece_side*6+piece]

            while bb:
                sq = lsb(bb)
                sq = flip_sq(sq) if king_side == BLACK else sq

                indices.append(king_sq*64*10 + (rel_side*5+piece)*64 + sq)

                bb &= bb-1

    return indices

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
        raw = self.mm[offset : offset + RECORD_SIZE]
        fields = struct.unpack_from(RECORD_FORMAT, raw)

        bitboards = fields[:12]
        score = fields[12]
        outcome = fields[13] * 0.5 + 0.5

        white = bitboards_to_indices(bitboards, 0)
        black = bitboards_to_indices(bitboards, 1)

        wdl_score = torch.sigmoid(torch.tensor(score, dtype=torch.float32).div_(400.0))
        target = 0.7 * wdl_score + 0.3 * outcome

        return torch.tensor(white, dtype=torch.int32), torch.tensor(black, dtype=torch.int32), target

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()

        self.l1 = nn.Linear(10*64*64, 256)
        self.l2 = nn.Linear(512, 32)
        self.l3 = nn.Linear(32, 1)

    def feed_l1(self, batch_size, features, indices):
        f = self.l1.weight[:, features].T
        a1 = torch.zeros(batch_size, 256, device=features.device)
        a1.index_add_(0, indices, f)
        return a1 + self.l1.bias

    def forward(self, batch_size, white_features, white_indices, black_features, black_indices):
        a1w = self.feed_l1(batch_size, white_features, white_indices)
        a1b = self.feed_l1(batch_size, black_features, black_indices)

        a1 = torch.concat([a1w, a1b], dim=1)
        a1 = F.hardtanh(a1, 0, 1)

        a2 = F.hardtanh(self.l2(a1), 0, 1)
        a3 = self.l3(a2)

        return a3.squeeze(-1)
