#include <cstdio>
#include <cmath>
#include <algorithm>
#include <bit>

#include "blunderfish.h"
#include "nnue_embed.h"

static constexpr int32_t RELU_Q = 127;
static constexpr int NNUE_INPUT_FEAUTURES = 64*10*64;

static_assert(ACCUMULATOR_SIZE == NNUE_ACCUMULATOR_SIZE);

inline int32_t scaled_crelu(int32_t x, int32_t q) {
    return std::clamp(x*RELU_Q/q, 0, RELU_Q);
}

inline float scaled_sigmoid(int32_t x, int32_t q) {
    float xf = float(x)/float(q);
    return 1.0f / (1.0f + std::exp(-xf));
}

inline int flip_sq(int sq) {
    return sq ^ 56;
}

static std::vector<int> get_king_perspective_indices(std::span<uint64_t> bbs, int king_side) {
    std::vector<int> indices;

    int king_index[] = {
        5,
        11
    };

    int king_sq = std::countr_zero(bbs[king_index[king_side]]);
    if (king_side == BLACK) {
        king_sq = flip_sq(king_sq);
    }

    for (int piece_side = 0; piece_side < 2; ++piece_side) {
        int rel_side = king_side == BLACK ? 1-piece_side : piece_side;

        for (int piece = 0; piece < 5; ++piece) {
            uint64_t bb = bbs[piece_side*6+piece];

            for (int sq : set_bits(bb)) {
                int rel_sq = king_side == BLACK ? flip_sq(sq) : sq;
                int idx = king_sq*64*10+(rel_side*5+piece)*64 + rel_sq;
                indices.push_back(idx);
            }
        }
    }

    return indices;
}

static void feed_l1(std::span<int32_t> a0, std::span<int> set_indices) {
    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = nnue_b0[i];

        for (int j : set_indices) {
            a0[i] += int32_t(nnue_w0[j][i]);
        }
    }
}

float nnue_infer(std::span<uint64_t> bbs) {
    std::array<int32_t, std::size(nnue_b0)*2> a0;

    auto white_persp = get_king_perspective_indices(bbs, WHITE);
    auto black_persp = get_king_perspective_indices(bbs, BLACK);

    feed_l1(std::span(a0).subspan(0, std::size(nnue_b0)), white_persp);
    feed_l1(std::span(a0).subspan(std::size(nnue_b0), std::size(nnue_b0)), black_persp);

    for (auto& x : a0) {
        x = scaled_crelu(x, 64);
    }

    int32_t a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
        a1[i] = nnue_b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            a1[i] += int32_t(nnue_w1[i][j]) * int32_t(a0[j]);
        }

        a1[i] = scaled_crelu(a1[i], 64*127);
    }

    int32_t out = nnue_b2[0];

    for (size_t j = 0; j < std::size(a1); ++j) {
        out += int32_t(nnue_w2[0][j]) * int32_t(a1[j]);
    }

    return scaled_sigmoid(out, 64*127);
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

    auto white_persp = get_king_perspective_indices(bbs, WHITE);
    auto black_persp = get_king_perspective_indices(bbs, BLACK);

    feed_l1(std::span(accumulator).subspan(0, std::size(nnue_b0)), white_persp);
    feed_l1(std::span(accumulator).subspan(std::size(nnue_b0), std::size(nnue_b0)), black_persp);
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

static void update_accumulator_from_king_perspective(std::span<int32_t> accumulator, Piece captured_piece, int captured_pos, Piece moving_piece_start, Piece moving_piece_end, int move_from, int move_to, int rook_from, int rook_to, int side, int sign, int king_sq, int king_side) {
    // move the piece and castle rook moves

    auto accumulator_half = accumulator.subspan(king_side==BLACK ? ACCUMULATOR_SIZE/2 : 0, ACCUMULATOR_SIZE/2);

    if (king_side == BLACK) {
        king_sq = flip_sq(king_sq);
    }

    auto feature = [king_side, king_sq](Piece piece, int sq, int piece_side){
        if (king_side == BLACK) {
            sq = flip_sq(sq);
            piece_side = opponent(piece_side);
        }

        return king_sq*64*10 + (piece_side*5+bbs_piece_index[piece])*64 + sq;
    };

    for (size_t i = 0; i < accumulator_half.size(); ++i) {
        if (moving_piece_start != PIECE_KING) {
            accumulator_half[i] -= sign * int32_t(nnue_w0[feature(moving_piece_start, move_from, side)][i]);
            accumulator_half[i] += sign * int32_t(nnue_w0[feature(moving_piece_end, move_to, side)][i]);
        }

        if (rook_from != rook_to) {
            accumulator_half[i] -= sign * int32_t(nnue_w0[feature(PIECE_ROOK, rook_from, side)][i]);
            accumulator_half[i] += sign * int32_t(nnue_w0[feature(PIECE_ROOK, rook_to, side)][i]);
        }
    }

    // remove the captured piece

    if (captured_piece != PIECE_NONE) {
        for (size_t i = 0; i < accumulator_half.size(); ++i) {
            accumulator_half[i] -= sign * int32_t(nnue_w0[feature(captured_piece, captured_pos, opponent(side))][i]);
        }
    }
}

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
    int white_king_sq = std::countr_zero(sides[WHITE].bb[PIECE_KING]);
    int black_king_sq = std::countr_zero(sides[BLACK].bb[PIECE_KING]);

    if (moving_piece_start == PIECE_KING) {
        auto bbs = to_bitboards();

        auto persp = get_king_perspective_indices(bbs, side);
        feed_l1(std::span(accumulator).subspan(side == BLACK ? std::size(nnue_b0) : 0, std::size(nnue_b0)), persp);

        update_accumulator_from_king_perspective(accumulator, captured_piece, captured_pos, moving_piece_start, moving_piece_end, move_from, move_to, rook_from, rook_to, side, sign, side == BLACK ? white_king_sq : black_king_sq, opponent(side));
    }
    else {
        update_accumulator_from_king_perspective(accumulator, captured_piece, captured_pos, moving_piece_start, moving_piece_end, move_from, move_to, rook_from, rook_to, side, sign, white_king_sq, WHITE);
        update_accumulator_from_king_perspective(accumulator, captured_piece, captured_pos, moving_piece_start, moving_piece_end, move_from, move_to, rook_from, rook_to, side, sign, black_king_sq, BLACK);
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
    int32_t a0[std::size(nnue_b0)*2];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = scaled_crelu(accumulator[i], 64);
    }

    int32_t a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
        a1[i] = nnue_b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            a1[i] += int32_t(nnue_w1[i][j]) * int32_t(a0[j]);
        }

        a1[i] = scaled_crelu(a1[i], 64*127);
    }

    int32_t out = nnue_b2[0];

    for (size_t j = 0; j < std::size(a1); ++j) {
        out += int32_t(nnue_w2[0][j]) * int32_t(a1[j]);
    }

    eval_cache = wdl_to_centipawns(scaled_sigmoid(out, 64*127));
#else
    eval_cache = compute_eval();
#endif
    return *eval_cache;
}