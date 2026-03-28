from model import *

import chess
import subprocess
import math

model = NNUE()
state_dict = torch.load("model_epoch2_val0.489208.pt", map_location="cpu")
state_dict = {k.replace("_orig_mod.", ""): v for k, v in state_dict.items()}
model.load_state_dict(state_dict)
model.eval()

fens = [
    "r1b1kbnr/pppp1p2/B4q2/4p1pp/Qn6/P1P1PP1N/1P1P2PP/RNBK3R w kq - 2 1",
    "r2qk1nr/ppp2p2/2npb1p1/2b1p2p/P3P2P/R1P2PP1/1PQP4/1NB1KBNR w Kkq h6 2 1",
    "rn1qkb1r/1ppnppp1/p3b2p/8/1P1p4/1Q2PPP1/P1PP3P/RNB1KBNR w KQkq - 2 1",
    "rnb1qbnr/1pp1k1p1/4p3/p2p1p1p/2PP1PB1/N5P1/PP2P2P/R1BQ1KNR w - - 2 1",
    "r1bqkb1r/p1pp1p1p/Rp2p2n/8/5Bp1/3P1PPB/1PP1P2P/1N1QK1NR w Kkq - 2 1",
    "1rb1k1nr/2pp1pbp/ppn5/4p1p1/PPP1P1Pq/B2B4/3PQP1P/RN2K1NR w KQk - 2 1",
    "rnbqk2r/pppp4/2n1ppp1/bB5p/4P1PP/1PP5/P2PKP2/RNBQ2NR w kq - 2 1",
    "rn1k1bnr/p2pqp2/b1p1p2p/Pp4p1/5PP1/7N/1PPPP2P/RNBQKB1R w KQ - 2 1",
    "rn1q2nr/pbppb1p1/1P3pk1/4p2p/6P1/8/PPNPPP1P/1RBQKBNR w K - 2 1",
    "r1bqkbnr/p2p2p1/n5P1/2p1pp1p/RP5P/5P2/2PPP3/1NBQKBNR w Kkq - 2 1",
    "1rb1kbnr/nppqp2p/p2p1pp1/8/1QP2P2/4P3/PP1P2PP/RNB1KBNR w KQk - 2 1",
    "1rbqkb1r/pppp1n1p/4pp2/6p1/1PPn4/6PN/P2PPP1P/RNBQKB1R w Qk - 2 1",
    "1nb1kbnr/r1q1ppp1/3p3p/ppp5/2P3PP/P4P2/1P1PP3/RNBQKBNR w Qk - 2 1",
    "rnbqkb1r/4nppp/p1ppp3/1p6/3P1P2/1P3N2/P1P1PKPP/RNBQ1B1R w kq - 2 1",
    "r1bqkbnr/p1pp2p1/1p2p3/5p1p/1nP2P2/1QN5/PP1PPKPP/R1B2BNR w kq f6 2 1",
    "rn1qkb2/Np2npp1/3pb2r/2p1p3/P1P3Pp/5P1B/1P1PP2P/R1BQK1NR w KQq - 2 1",
    "rn2kbnr/ppqbp2p/3p2p1/2p2p2/B3P3/1P2Q3/P1PP1PPP/RNB1K1NR w KQkq - 2 1",
    "rnbq1bn1/1pppkpp1/p3r3/1P5p/3p1B1P/N6N/P1P1PPP1/R2QKB1R w KQ - 2 1",
    "r2qkb1r/pbppppp1/np3n2/8/1PPP1P1p/1Q2BN2/P3P1PP/RN2KB1R w KQq - 2 1",
    "rnq1kbnr/p1pp1p1p/b5p1/1p2pB2/8/1PN1PP2/P1PPK1PP/R1B1Q1NR w kq - 2 1",
    "r2qkb1r/1p2pppp/n1pp4/p7/3P3n/NQP4b/PP2PPPP/R1B1KB1R w KQkq - 2 1",
    "rnbqkb1r/p1p2p1p/3p4/1p2p1p1/1PQP1P1n/N4N2/P1P1P1PP/R1B1KB1R w KQkq - 2 1",
    "r1bq2nr/ppp1kpb1/4p3/2np2pp/3NP1PP/2N5/PPPP1PB1/R1BQK2R w KQ - 2 1",
    "r2qkbnr/pp2p1p1/4b3/1Ppp1p1p/2n4P/1Q6/P2PPPP1/R1BNKBNR w KQkq c6 2 1",
    "rnb1kbn1/p2qp3/1pp2ppr/3p3p/1B6/3P4/PPP1PPPP/RNQ1KBNR w KQq - 2 1",
    "rnb1kb2/p3qppr/2p2n1p/1p1pp3/P1P1P1P1/5P2/1P1P2BP/RNBQK1NR w q d6 2 1",
    "rnb1kb1r/p2p1p2/3qpnp1/1pp4p/1P1P4/2P1P2P/P2QBPPR/RNB1K1N1 w Qkq b6 2 1",
    "rnb1kb1r/p2pqpp1/1pp2n2/4p1Bp/1P1P2P1/2P5/P1Q1PPBP/RN2K1NR w KQkq - 2 1",
    "rnbqkb1r/1p2npp1/p2p3p/4p3/2pP1PP1/PPP4B/R3P2P/1NBQK1NR w Kkq - 2 1",
    "rnbqk2r/p1pp1pp1/1p1b3p/4p2n/7P/1QPPPP1R/PP1B2P1/RN2KBN1 w Qkq - 2 1",
    "rn1qkbnr/4p3/1ppp1ppp/p4b2/PP3P2/6PN/N1PPPK1P/R1BQ1B1R w kq - 2 1",
    "r1bqk1nr/pppp1pbp/4p3/4n1p1/2P5/3PP1PP/PP1K1P2/RNBQ1BNR w kq - 2 1",
    "r1b1kbr1/p1qppp1p/n1p2n2/Q5p1/p7/1P1PBP1N/2P1P1PP/RN2KB1R w KQq - 2 1",
    "2rqkbnr/p1p1pp2/3pb1pp/1pn5/2P5/4QPP1/PP1PP2P/RNB1KBNR w KQk b6 2 1",
    "rnb1kbnr/p2p2p1/1qp5/1p2pp1p/4P2P/N7/PPPPQPP1/1RB1KBNR w Kkq e6 2 1",
    "r2qkbnr/1bpp1p2/1p2p2p/p1n1N1p1/P6P/2NP4/1PPKPPP1/R1BQ1B1R w kq - 2 1",
    "1rbqkb1r/1ppnpp1p/7B/p2p3n/6p1/3P4/PPP1PPPP/RN1QKBNR w KQk - 2 1",
    "rnb1qb1r/p2pkppp/4p3/1p3n2/3p2P1/PP2P2N/2PPQP1P/RN2KB1R w KQ - 2 1",
    "r2qkb1r/pppnpp1p/Q1n1b3/3p2p1/1P1P4/2P2N1B/P3PP1P/RNB1K2R w KQkq g6 2 1",
    "1nb1kbnr/rp1p1pp1/1qp1p2p/8/p4P2/P2P3P/1PPQP1PR/RNBK1BN1 w k - 2 1",
    "r1bqkbnr/p3p1p1/np6/2pp1p1p/1P6/P4PNP/2PPP1P1/R1BQKBNR w KQkq - 2 1",
    "r1b1k1nr/pp1pb1p1/n4p2/q1pBp3/6Pp/7N/PPPPPP1P/RNBQK2R w KQkq - 2 1",
    "rn1qkbnr/p5p1/bpp1pp1p/3p4/1P1P1P2/2P5/PB2P1PP/RN1QKBNR w KQkq - 2 1",
    "rnb1kbnr/2p2pp1/ppq4p/3pP3/P7/1PNP3N/2P1P1PP/R1BQKB1R w KQkq - 2 1",
    "rnb1k1nr/1p1pqpp1/2pb4/p3p2p/2P5/4P1PP/PPNP1P2/R1BQKBNR w KQkq - 2 1",
    "1rbqkbr1/pppppp1p/n5p1/5n2/2P1Q3/5N2/PPNPPPPP/R1BK1B1R w - - 2 1",
    "rnbk1bnr/1pq3pp/p1p2p2/3pp3/2P1B1P1/PQ2PP2/1P1P3P/RNB1K1NR w KQ - 2 1",
    "rnb1kbnr/ppq1p1p1/2p4p/P1Bp1p2/8/2PP1P2/1P1NP1PP/R2QKBNR w KQq - 2 1",
    "2bqk1nr/rp1p3p/p1nb2p1/1Pp1pp2/8/P1P1PP1P/R2P2P1/1NBQKBNR w Kk - 2 1",
    "r1bk1bnr/ppp1q1pp/2n2p2/3pp3/3PPB2/P6N/1PPQ1PPP/RN2KB1R w KQ - 2 1",
    "rn1q1b1r/3kpppp/b2p4/Pppn4/3P2P1/N4P1P/P1P1P3/1RBQKBNR w K - 2 1",
    "rn1qkbnr/p3ppp1/2pp4/1p1b1B1P/1P1P4/P7/1BP1PP1P/RN1QK1NR w KQkq - 2 1",
    "1rb1kb1r/pp2pp1p/n2p1np1/2p2q2/2P5/NP6/PB1PPPPP/2RQKBNR w Kk - 2 1",
    "r1bqkbnr/p1p1pp2/n2p4/1p4pp/5PP1/N2P3P/PPP1PK2/1RBQ1BNR w q - 2 1",
    "1n1qkbnr/1p2pppp/r7/2pp1b2/p3P2P/N1P2N2/PP1P1PP1/R1BQK2R w Kk - 2 1",
]

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

for f in fens:
    white_features, white_indices, black_features, black_indices = fen_to_inputs(f)
    logits = model(1, white_features, white_indices, black_features, black_indices).item()
    y = sigmoid(logits)
    z = cpp_eval(f)
    print(f"torch: {y}, cpp: {z}")