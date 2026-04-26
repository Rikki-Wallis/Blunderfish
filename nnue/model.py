import struct
import mmap
import os
import numpy as np
import math

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

N_EMBED = 256

class AttentionBlock(nn.Module):
    def __init__(self):
        super().__init__()
        self.query = nn.Linear(N_EMBED, N_EMBED, bias=False)
        self.key   = nn.Linear(N_EMBED, N_EMBED, bias=False)
        self.value = nn.Linear(N_EMBED, N_EMBED, bias=False)
        self.norm  = nn.LayerNorm(N_EMBED)
        self.ff    = nn.Sequential(
            nn.Linear(N_EMBED, N_EMBED * 4),
            nn.ReLU(),
            nn.Linear(N_EMBED * 4, N_EMBED)
        )
        self.norm2  = nn.LayerNorm(N_EMBED)

    def forward(self, x, mask):
            # attention + residual
            q, k, v = self.query(x), self.key(x), self.value(x)
            wei = q @ k.transpose(-1, -2) / math.sqrt(N_EMBED)

            if mask is not None:
                wei = wei + mask

            wei = F.softmax(wei, dim=-1)
            x = x + wei @ v

            # feedforward + residual
            x = self.norm(x)
            x = x + self.ff(x)
            x = self.norm2(x)
            return x

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()

        self.embedding_table = nn.Embedding(769, N_EMBED, padding_idx=768)

        self.blocks = nn.ModuleList([AttentionBlock(), AttentionBlock(), AttentionBlock(), AttentionBlock()])

        self.linear = nn.Linear(N_EMBED, 1)
        self.cls = nn.Parameter(torch.randn(1, 1, N_EMBED))

    def forward(self, features):
        B = features.shape[0]

        is_pad = features == 768

        cls_is_pad = torch.zeros(B, 1, dtype=torch.bool, device=features.device)
        full_pad_mask = torch.cat([cls_is_pad, is_pad], dim=1)

        attn_mask = torch.zeros_like(full_pad_mask, dtype=torch.float32)
        attn_mask = attn_mask.masked_fill(full_pad_mask, -1e9)

        attn_mask = attn_mask.unsqueeze(1)

        tokens = self.embedding_table(features) # (B, N, D)
        cls = self.cls.expand(B, -1, -1)
        tokens = torch.cat([cls, tokens], dim=1)

        x = tokens
        for block in self.blocks:
            x = block(x, mask=attn_mask)

        pooled = x[:, 0]
        eval = F.sigmoid(self.linear(pooled)) # (B, 1)

        return eval.squeeze(-1)
