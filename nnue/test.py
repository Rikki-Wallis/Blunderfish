from model import *

import chess
import struct
import subprocess
import math
import matplotlib.pyplot as plt
import seaborn as sns

from scipy.stats import spearmanr

RECORD_FORMAT = "=12Qii"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

def load_model(path):
    model = NNUE()
    state_dict = torch.load(path, map_location="cpu")
    state_dict = {k.replace("_orig_mod.", ""): v for k, v in state_dict.items()}
    model.load_state_dict(state_dict)
    model.eval()
    return model

old_model = load_model("legacy.pt")
model = load_model("new.pt")

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

#for f in fens:
#    white_features, white_indices, black_features, black_indices = fen_to_inputs(f)
#    logits = model(1, white_features, white_indices, black_features, black_indices).item()
#    y = sigmoid(logits)
#    z = cpp_eval(f)
#    print(f"torch: {y}, cpp: {z}")

class DatasetIterator:
    def __init__(self, dataset):
        self.dataset = dataset
        self.index = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.index < len(self.dataset):
            val = self.dataset[self.index]
            self.index += 1
            return val
        raise StopIteration

class Dataset:
    def __init__(self):
        with open("aggregate.bin", "rb") as f:
            self.data = f.read()
            self.n = len(self.data)//RECORD_SIZE

    def __getitem__(self, idx):
        assert idx < self.n
        offset = idx * RECORD_SIZE
        fields = struct.unpack_from(RECORD_FORMAT, self.data, offset)

        bbs = fields[:12]
        score = fields[12]
        outcome = fields[13]

        return bbs, score, outcome

    def __len__(self):
        return self.n

    def __iter__(self):
        return DatasetIterator(self)

data = Dataset()

actual_scores = []
old_scores = []
new_scores = []

for i in range(0, len(data), 1000):
    bbs, score, outcome = data[i]

    white_features, white_indices, black_features, black_indices = bitboards_to_inputs(bbs)

    actual_scores.append(score)

    for m, vec in zip([old_model, model], [old_scores, new_scores]):
        logits = m(1, white_features, white_indices, black_features, black_indices).item()
        wdl = sigmoid(logits)
        centipawns = wdl_to_centipawns(wdl)
        vec.append(centipawns)

print(f"Mean score actual: {sum(actual_scores)/len(actual_scores)}")
print(f"Mean score old: {sum(old_scores)/len(old_scores)}")
print(f"Mean score new: {sum(new_scores)/len(new_scores)}")

corr_old = spearmanr(old_scores, actual_scores)
corr_new = spearmanr(new_scores, actual_scores)

print(f"Correlation old: {corr_old}")
print(f"Correlation new: {corr_new}")

sns.kdeplot(actual_scores, label="Actual data")
sns.kdeplot(old_scores, label="Old model")
sns.kdeplot(new_scores, label="New model")
plt.xlabel("Score")
plt.ylabel("Density")
plt.title("Score Distribution Comparison")
plt.legend()
plt.savefig("scores.png")
