#include <cstdio>
#include <cmath>

#include "blunderfish.h"

static NNUE load_nnue() {
#ifdef LOAD_NNUE
    NNUE nn;

    FILE* file = fopen("/home/lyndon/dev/nn/model.bin", "rb");

    if (!file) {
        print("Failed to load nnue.\n");
        return {};
    }

    fread(nn.w0, sizeof(nn.w0), 1, file);
    fread(nn.b0, sizeof(nn.b0), 1, file);
    fread(nn.w1, sizeof(nn.w1), 1, file);
    fread(nn.b1, sizeof(nn.b1), 1, file);
    fread(nn.w2, sizeof(nn.w2), 1, file);
    fread(nn.b2, sizeof(nn.b2), 1, file);

    fclose(file);

    return nn;
#else
    return {};
#endif
}

NNUE nnue = load_nnue();

inline float relu(float x) {
    return std::max(x, 0.0f);
}

inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float nnue_infer(std::span<uint64_t> bbs) {
    float input[768] = {};

    for (int side = 0; side < 2; ++side) {
        for (int p = 0; p < 6; ++p) {
            for (int x : set_bits(bbs[side*6+p])) {
                input[(side*6+p)*64+x] = 1.0f;
            }
        }
    }

    float a0[256];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = nnue.b0[i];

        for (size_t j = 0; j < std::size(input); ++j) {
            a0[i] += nnue.w0[i][j] * input[j];
        }

        a0[i] = relu(a0[i]);
    }

    float a1[32];

    for (size_t i = 0; i < std::size(a1); ++i) {
        a1[i] = nnue.b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            a1[i] += nnue.w1[i][j] * a0[j];
        }

        a1[i] = relu(a1[i]);
    }

    float out = nnue.b2[0];
    for (size_t j = 0; j < std::size(a1); ++j) {
        out += nnue.w2[0][j] * a1[j];
    }

    return sigmoid(out);
}

int64_t Position::nnue_eval() const {
    uint64_t bbs[12];

    Piece pieces[6] = {
        PIECE_PAWN,
        PIECE_KNIGHT,
        PIECE_BISHOP,
        PIECE_ROOK,
        PIECE_QUEEN,
        PIECE_KING
    };

    for (int side = 0; side < 2; ++side) {
        for (int pid = 0; pid < 6; ++pid) {
            bbs[side*6+pid] = sides[side].bb[pieces[pid]];
        }
    }

    float wdl = nnue_infer(bbs);
    int64_t centipawns = int64_t(400.0f * logf(wdl/(1.0f-wdl)));

    return centipawns;
}