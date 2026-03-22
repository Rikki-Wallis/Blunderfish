#include <cstdio>
#include <cmath>
#include <algorithm>

#include "blunderfish.h"
#include "nnue_embed.h"

static_assert(ACCUMULATOR_SIZE == NNUE_ACCUMULATOR_SIZE);

inline float crelu(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}

inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

inline std::array<float, NNUE_INPUT_FEATURES> bbs_to_input(std::span<uint64_t> bbs) 
{
    std::array<float, NNUE_INPUT_FEATURES> input{};

    for (int side = 0; side < 2; ++side) {
        for (int p = 0; p < 6; ++p) {
            for (int x : set_bits(bbs[side*6+p])) {
                input[(side*6+p)*64+x] = 1.0f;
            }
        }
    }

    return input;
}

float nnue_infer(std::span<uint64_t> bbs) {
    auto input = bbs_to_input(bbs);

    float a0[std::size(nnue_b0)];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = nnue_b0[i];

        for (size_t j = 0; j < std::size(input); ++j) {
            a0[i] += nnue_w0[j][i] * input[j];
        }

        a0[i] = crelu(a0[i]);
    }

    float a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
        a1[i] = nnue_b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            a1[i] += nnue_w1[i][j] * a0[j];
        }

        a1[i] = crelu(a1[i]);
    }

    float out = nnue_b2[0];
    for (size_t j = 0; j < std::size(a1); ++j) {
        out += nnue_w2[0][j] * a1[j];
    }

    return sigmoid(out);
}

inline int64_t wdl_to_centipawns(float wdl) {
    wdl = std::clamp(wdl, 1e-7f, 1.0f - 1e-7f);
    int64_t centipawns = int64_t(400.0f * logf(wdl/(1.0f-wdl)));
    return centipawns;
}

int64_t Position::nnue_eval() const {
    auto bbs = to_bitboards();
    float wdl = nnue_infer(bbs);
    return wdl_to_centipawns(wdl);
}

void Position::reset_nnue_accumulator() {
#ifdef USE_NNUE
    auto bbs = to_bitboards();
    auto input = bbs_to_input(bbs);

    for (size_t i = 0; i < std::size(accumulator); ++i) {
        accumulator[i] = nnue_b0[i];

        for (size_t j = 0; j < input.size(); ++j) {
            accumulator[i] += nnue_w0[j][i] * input[j];
        }
    }
#endif
}

static const size_t bbs_piece_index[NUM_PIECE_TYPES] = {
    0xffffffff,
    0,
    3,
    1,
    2,
    4,
    5,
}; 

void Position::update_eval(Piece captured_piece, int captured_pos, Piece moving_piece_start, Piece moving_piece_end, int move_from, int move_to, int rook_from, int rook_to, int side, int sign) {
    (void)captured_piece;
    (void)captured_pos;
    (void)moving_piece_start;
    (void)moving_piece_end;
    (void)move_from;
    (void)move_to;
    (void)rook_from;
    (void)rook_to;
    (void)side;
    (void)sign;

#ifdef USE_NNUE
    auto feature = [](Piece piece, int pos, int side) {
        return (side * 6 + bbs_piece_index[piece])*64+pos;
    };

    float sf = float(sign);

    // move the piece and castle rook moves

    for (size_t i = 0; i < std::size(nnue_b0); ++i) {
        accumulator[i] -= sf * nnue_w0[feature(moving_piece_start, move_from, side)][i];
        accumulator[i] += sf * nnue_w0[feature(moving_piece_end, move_to, side)][i];

        accumulator[i] -= sf * nnue_w0[feature(PIECE_ROOK, rook_from, side)][i];
        accumulator[i] += sf * nnue_w0[feature(PIECE_ROOK, rook_to, side)][i];
    }

    // remove the captured piece

    if (captured_piece != PIECE_NONE) {
        for (size_t i = 0; i < std::size(nnue_b0); ++i) {
            accumulator[i] -= sf * nnue_w0[feature(captured_piece, captured_pos, opponent(side))][i];
        }
    }

#else
    //// NOTE: if rook_from == rook_to there is NO castle
    //// ensure that if that is the case, your castling operations have a NET ZERO

    //incremental_eval -= piece_delta(captured_piece, captured_pos, opponent(side));

    //incremental_eval -= piece_delta(moving_piece_start, move_from, side);
    //incremental_eval += piece_delta(moving_piece_end, move_to, side);

    //// since these are symmetric, should have net zero when rook_from == rook_to
    //incremental_eval -= piece_delta(PIECE_ROOK, rook_from, to_move);
    //incremental_eval += piece_delta(PIECE_ROOK, rook_to, to_move);
#endif
}

int64_t Position::get_eval() {
    if (eval_cache.has_value()) {
        return *eval_cache;
    }

#ifdef USE_NNUE
    float a0[std::size(nnue_b0)];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = crelu(accumulator[i]);
    }

    float a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
        a1[i] = nnue_b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            a1[i] += nnue_w1[i][j] * a0[j];
        }

        a1[i] = crelu(a1[i]);
    }

    float out = nnue_b2[0];

    for (size_t j = 0; j < std::size(a1); ++j) {
        out += nnue_w2[0][j] * a1[j];
    }

    eval_cache = wdl_to_centipawns(sigmoid(out));
#else
    eval_cache = compute_eval();
#endif
    return *eval_cache;
}