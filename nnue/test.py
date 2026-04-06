from model import *

import chess
import struct
import subprocess
import math
import matplotlib.pyplot as plt
import seaborn as sns

import fens

from scipy.stats import spearmanr

RECORD_FORMAT = "=12Qibb"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

def load_model(path):
    model = NNUE()
    state_dict = torch.load(path, map_location="cpu")
    state_dict = {k.replace("_orig_mod.", ""): v for k, v in state_dict.items()}
    model.load_state_dict(state_dict)
    model.eval()
    return model

model = load_model("model_epoch10_val0.010757.pt")

def fen_to_bitboards(fen):
    board = chess.Board(fen)

    pieces = [
        chess.PAWN,
        chess.KNIGHT,
        chess.BISHOP,
        chess.ROOK,
        chess.QUEEN,
        chess.KING
    ]

    sides = [
        chess.WHITE,
        chess.BLACK
    ]

    bitboards = [0]*12

    for si, side in enumerate(sides):
        for pi, p in enumerate(pieces):
            sqs = board.pieces(p, side)
            for sq in range(64):
                if sq in sqs:
                    bitboards[si*6+pi] |= (1<<sq)

    return bitboards

def bitboards_to_inputs(bbs):
    white_features = bitboards_to_indices(bbs, 0)
    black_features = bitboards_to_indices(bbs, 1)

    white_indices = torch.zeros(len(white_features),dtype=torch.long)
    black_indices = torch.zeros(len(black_features),dtype=torch.long)

    return (
        torch.tensor(white_features,dtype=torch.long),
        white_indices,
        torch.tensor(black_features,dtype=torch.long),
        black_indices,
    )

def fen_to_inputs(fen):
    bbs = fen_to_bitboards(fen)
    return bitboards_to_inputs(bbs)

def clamp(x, x0, x1):
    return min(max(x, x0), x1)

def wdl_to_centipawns(wdl):
    wdl = clamp(wdl, 1e-7, 1.0 - 1e-7)
    return int(400.0 * math.log(wdl/(1.0-wdl)))

def cpp_eval(fen):
    result = subprocess.run(["/home/lyndon/dev/Blunderfish/build/eval/eval", fen], capture_output=True, text=True)
    if result.returncode != 0:
        raise "cpp eval failed"
    return result.stdout.strip()

def sigmoid(x):
    return 1/(1+math.exp(-x))

for f in fens.fens:
    white_features, white_indices, black_features, black_indices = fen_to_inputs(f)
    y = model(white_features, white_indices, black_features, black_indices).item()
    z = cpp_eval(f)
    print(f"torch: {y}, cpp: {z}")