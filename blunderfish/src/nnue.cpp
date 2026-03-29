#include <cstdio>
#include <cmath>
#include <algorithm>
#include <bit>

#include <immintrin.h>

#include "blunderfish.h"
#include "nnue_embed.h"

//static int16_t nnue_w0[10][10];
//static int16_t nnue_b0[10];
//static int16_t nnue_w1[10][10];
//static int32_t nnue_b1[10];
//static int16_t nnue_w2[10][10];
//static int32_t nnue_b2[10];

static constexpr int32_t RELU_Q = 127;
static constexpr int NNUE_INPUT_FEAUTURES = 64*10*64;

static_assert(ACCUMULATOR_SIZE == NNUE_ACCUMULATOR_SIZE);

template<typename T>
inline T scaled_crelu(T x, T q) {
    T sx = x*T(RELU_Q)/q;
    T lo = 0;
    T hi = RELU_Q;
    return std::clamp(sx, lo, hi);
}

inline float scaled_sigmoid(int32_t x, int32_t q) {
    float xf = float(x)/float(q);
    return 1.0f / (1.0f + std::exp(-xf));
}

inline int flip_sq(int sq) {
    return sq ^ 56;
}

struct ActiveIndices {
    int data[64];
    int count;
};

static ActiveIndices get_king_perspective_indices(std::span<uint64_t> bbs, int king_side) {
    ActiveIndices indices;
    indices.count = 0;

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

                assert(indices.count < std::size(indices.data));
                indices.data[indices.count++] = idx;
            }
        }
    }

    return indices;
}

static void feed_l1(std::span<int16_t> a0, const ActiveIndices& set_indices) {
    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = nnue_b0[i];

        for (int ji = 0; ji < set_indices.count; ++ji) {
            int j = set_indices.data[ji];
            a0[i] += nnue_w0[j][i];
        }
    }
}

static float forward_accumulator(std::span<int16_t> accumulator) {
    alignas(32) int16_t a0[std::size(nnue_b0)*2];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = scaled_crelu(accumulator[i], int16_t(64));
    }

    int32_t a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
        __m256i sum256 = _mm256_setzero_si256(); // 8x int32s to zero

        for (size_t j = 0; j < std::size(a0); j += 16) {
            __m256i act = _mm256_load_si256((const __m256i*)&a0[j]);
            __m256i w = _mm256_load_si256((const __m256i*)&nnue_w1[i][j]);

            __m256i prod = _mm256_madd_epi16(act, w);
            sum256 = _mm256_add_epi32(sum256, prod);
        }

        __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum256), _mm256_extracti128_si256(sum256, 1));
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2)));
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2, 3, 0, 1)));

        a1[i] = nnue_b1[i] + _mm_cvtsi128_si32(sum128);
        a1[i] = scaled_crelu(a1[i], 64*127);
    }

    int32_t out = nnue_b2[0];

    for (size_t j = 0; j < std::size(a1); ++j) {
        out += int32_t(nnue_w2[0][j]) * int32_t(a1[j]);
    }

    return scaled_sigmoid(out, 64*127);
}


float nnue_infer(std::span<uint64_t> bbs) {
    std::array<int16_t, std::size(nnue_b0)*2> accumulator;

    auto white_persp = get_king_perspective_indices(bbs, WHITE);
    auto black_persp = get_king_perspective_indices(bbs, BLACK);

    feed_l1(std::span(accumulator).subspan(0, std::size(nnue_b0)), white_persp);
    feed_l1(std::span(accumulator).subspan(std::size(nnue_b0), std::size(nnue_b0)), black_persp);

    return forward_accumulator(accumulator);
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

static void move_piece_in_accumulator(std::span<int16_t> accumulator_half, size_t add_feature, size_t remove_feature, int sign) {
    if (sign == -1) {
        std::swap(add_feature, remove_feature);
    }

    for (size_t i = 0; i < accumulator_half.size(); i += 16) {
        __m256i a = _mm256_load_si256((__m256i*)(&accumulator_half[i]));
        __m256i add = _mm256_load_si256((__m256i*)&nnue_w0[add_feature][i]);
        __m256i sub = _mm256_load_si256((__m256i*)&nnue_w0[remove_feature][i]);

        a = _mm256_add_epi16(a, add);
        a = _mm256_sub_epi16(a, sub);

        _mm256_store_si256((__m256i*)(&accumulator_half[i]), a);
    }
}

static void update_accumulator_from_king_perspective(std::span<int16_t> accumulator, Piece captured_piece, int captured_pos, Piece moving_piece_start, Piece moving_piece_end, int move_from, int move_to, int rook_from, int rook_to, int side, int sign, int king_sq, int king_side) {
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

    // moving piece

    if (moving_piece_start != PIECE_KING) {
        move_piece_in_accumulator(accumulator_half, feature(moving_piece_end, move_to, side), feature(moving_piece_start, move_from, side), sign);
    }

    // rook movement for castling

    if (rook_from != rook_to) {
        move_piece_in_accumulator(accumulator_half, feature(PIECE_ROOK, rook_to, side), feature(PIECE_ROOK, rook_from, side), sign);
    }

    // remove the captured piece

    if (captured_piece != PIECE_NONE) {
        for (size_t i = 0; i < accumulator_half.size(); i += 16) {
            __m256i a = _mm256_load_si256((__m256i*)(&accumulator_half[i]));
            __m256i w = _mm256_load_si256((__m256i*)&nnue_w0[feature(captured_piece, captured_pos, opponent(side))][i]);

            if (sign == -1) {
                a = _mm256_add_epi16(a, w);
            }
            else {
                a = _mm256_sub_epi16(a, w);
            }

            _mm256_store_si256((__m256i*)(&accumulator_half[i]), a);
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
    float x = forward_accumulator(accumulator);
    eval_cache = wdl_to_centipawns(x);
#else
    eval_cache = compute_eval();
#endif
    return *eval_cache;
}