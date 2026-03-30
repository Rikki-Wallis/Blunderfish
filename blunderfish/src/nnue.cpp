#include <cstdio>
#include <cmath>
#include <algorithm>
#include <bit>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "blunderfish.h"
#include "nnue_embed.h"

//static int8_t nnue_w0[10][10];
//static int16_t nnue_b0[10];
//static int8_t nnue_w1[10][10];
//static int32_t nnue_b1[10];
//static int16_t nnue_w2[10][10];
//static int32_t nnue_b2[10];

static constexpr int NNUE_INPUT_FEAUTURES = 64*10*64;

static_assert(ACCUMULATOR_SIZE == NNUE_ACCUMULATOR_SIZE);

template<typename A, typename B, size_t OUT_Q>
inline B scaled_crelu(A x, A q) {
    A sx = x*A(OUT_Q)/q;
    A lo = 0;
    A hi = OUT_Q;
    return B(std::clamp(sx, lo, hi));
}

inline float sigmoid(float x) {
    return 1.0f/(1+std::exp(-x));
}

inline float scaled_sigmoid(int32_t x, int32_t q) {
    float xf = float(x)/float(q);
    return sigmoid(xf);
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

static void feed_l1(int16_t* RESTRICT a0, int* RESTRICT indices, int index_count) {
    // initialize with bias
    const size_t N = ACCUMULATOR_SIZE/2;

    for (size_t i = 0; i < N; ++i) {
        a0[i] = nnue_b0[i];
    }

    // add weights
    for (int ji = 0; ji < index_count; ++ji) {
        int j = indices[ji];

        for (size_t i = 0; i < N; ++i) {
            a0[i] += int16_t(nnue_w0[j][i]);
        }
    }
}

static float forward_accumulator(int16_t* RESTRICT accumulator) {
    alignas(32) uint8_t a0[std::size(nnue_b0)*2];

    for (size_t i = 0; i < std::size(a0); ++i) {
        a0[i] = scaled_crelu<int16_t, uint8_t, 255>(accumulator[i], int16_t(64));
    }

    alignas(32) uint8_t a1[std::size(nnue_b1)];

    for (size_t i = 0; i < std::size(a1); ++i) {
#if defined(__AVX2__) && true
        __m256i ones = _mm256_set1_epi16(1);
        __m256i sum  = _mm256_setzero_si256();  // int32 accumulator

        for (size_t j = 0; j < std::size(a0); j += 32) {
            __m256i act = _mm256_load_si256((const __m256i*)&a0[j]);      // uint8
            __m256i w   = _mm256_load_si256((const __m256i*)&nnue_w1[i][j]); // int8

            __m256i prod = _mm256_maddubs_epi16(act, w);  // uint8*int8 → int16 (saturating), 32 pairs
            __m256i wide = _mm256_madd_epi16(prod, ones); // int16*1 → int32, 16 pairs summed
            sum = _mm256_add_epi32(sum, wide);
        }

        __m128i lo  = _mm256_castsi256_si128(sum);
        __m128i hi  = _mm256_extracti128_si256(sum, 1);
        __m128i s   = _mm_add_epi32(lo, hi);
        s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1,0,3,2)));
        s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2,3,0,1)));
        int32_t value = _mm_cvtsi128_si32(s) + nnue_b1[i];
#else
        int32_t value = nnue_b1[i];

        for (size_t j = 0; j < std::size(a0); ++j) {
            value += int16_t(nnue_w1[i][j]) * int16_t(a0[j]);
        }
#endif

        a1[i] = scaled_crelu<int32_t, uint8_t, 255>(value, 64*255);
    }

    int32_t out = nnue_b2[0];

    for (size_t j = 0; j < std::size(a1); ++j) {
        out += int16_t(nnue_w2[0][j]) * int16_t(a1[j]);
    }

    return scaled_sigmoid(out, 64*255);
}


float nnue_infer(std::span<uint64_t> bbs) {
    std::array<int16_t, std::size(nnue_b0)*2> accumulator;

    auto white_persp = get_king_perspective_indices(bbs, WHITE);
    auto black_persp = get_king_perspective_indices(bbs, BLACK);

    feed_l1(accumulator.data(), white_persp.data, white_persp.count);
    feed_l1(accumulator.data() + ACCUMULATOR_SIZE / 2, black_persp.data, black_persp.count);

    return forward_accumulator(accumulator.data());
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

    feed_l1(accumulator, white_persp.data, white_persp.count);
    feed_l1(accumulator + ACCUMULATOR_SIZE / 2, black_persp.data, black_persp.count);
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

template<int N_ADDS, int N_SUBS>
ALWAYS_INLINE void update_accumulator_half(int16_t* RESTRICT accumulator_half, const size_t* adds, const size_t* subs) {
    for (size_t i = 0; i < ACCUMULATOR_SIZE / 2; ++i) {
        int16_t val = accumulator_half[i];

        for (size_t j = 0; j < N_ADDS; ++j) {
            val += int16_t(nnue_w0[adds[j]][i]);
        }

        for (size_t j = 0; j < N_SUBS; ++j) {
            val -= int16_t(nnue_w0[subs[j]][i]);
        }

        accumulator_half[i] = val;
    }
}

static void update_accumulator_from_king_perspective(int16_t* RESTRICT accumulator, Piece captured_piece, int captured_pos, Piece moving_piece_start, Piece moving_piece_end, int move_from, int move_to, int rook_from, int rook_to, int side, int sign, int king_sq, int king_side) {
    // move the piece and castle rook moves

    auto accumulator_half = king_side==BLACK ? accumulator + ACCUMULATOR_SIZE/2 : accumulator;

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


    bool king_move = moving_piece_start == PIECE_KING;
    bool is_castle = rook_from != rook_to;
    bool is_capture = captured_piece != PIECE_NONE;

    // moving piece

    if (!king_move) {
        if (is_capture) {
            size_t adds[] = {
                feature(moving_piece_end, move_to, side),
            };

            size_t subs[] = {
                feature(moving_piece_start, move_from, side),
                feature(captured_piece, captured_pos, opponent(side))
            };

            if (sign != -1) {
                update_accumulator_half<std::size(adds), std::size(subs)>(accumulator_half, adds, subs);
            }
            else {
                update_accumulator_half<std::size(subs), std::size(adds)>(accumulator_half, subs, adds);
            }
        }
        else {
            size_t adds[] = {
                feature(moving_piece_end, move_to, side),
            };

            size_t subs[] = {
                feature(moving_piece_start, move_from, side),
            };

            if (sign != -1) {
                update_accumulator_half<std::size(adds), std::size(subs)>(accumulator_half, adds, subs);
            }
            else {
                update_accumulator_half<std::size(subs), std::size(adds)>(accumulator_half, subs, adds);
            }
        }
    }
    else {
        if (is_castle) {
            size_t adds[] = {
                feature(PIECE_ROOK, rook_to, side)
            };

            size_t subs[] = {
                feature(PIECE_ROOK, rook_from, side)
            };

            if (sign != -1) {
                update_accumulator_half<std::size(adds), std::size(subs)>(accumulator_half, adds, subs);
            }
            else {
                update_accumulator_half<std::size(subs), std::size(adds)>(accumulator_half, subs, adds);
            }
        }
        else if (is_capture) {
            size_t subs[] = {
                feature(captured_piece, captured_pos, opponent(side))
            };

            if (sign != -1) {
                update_accumulator_half<0, std::size(subs)>(accumulator_half, nullptr, subs);
            }
            else {
                update_accumulator_half<std::size(subs), 0>(accumulator_half, subs, nullptr);
            }
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
        feed_l1(accumulator + (side == BLACK ? ACCUMULATOR_SIZE/2 : 0), persp.data, persp.count);

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