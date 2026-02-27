#include <cstdio>
#include <vector>
#include <cstdint>
#include <random>
#include "blunderfish.h"

void Position::initialize_zobrist_tables() {
    /** 
        VERY IMPORTANT TO ONLY RUN AT START OF PROGRAM.
        This method sets up all of the piece tables and
        initialises the zobrist hash and thus is very slow.
    */
    // Create and seed rand num gen
    std::mt19937_64 rng(8008135);
    std::uniform_int_distribution<uint64_t> dist(std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());

    // Set random numbers for each square for each piece
    for (int side = 0; side < 2; ++side) {
        for (int piece = PIECE_PAWN; piece < NUM_PIECE_TYPES; ++piece) {
            for (int sq = 0; sq < 64; ++sq) {
                zobrist_piece[side][piece][sq] = rng();
            }
        }
    }      

    // Set random number for black to move
    zobrist_side = rng();

    // Set random numbers for position flags

    for (int i = 0; i < 8; ++i) { // all singular bits
        zobrist_flags[1 << i] = rng();
    }

    for (size_t i = 0; i < std::size(zobrist_flags); ++i) { // all combinations of singular bits
        uint64_t x = 0;

        for (int b : set_bits((uint64_t)i)) {
            x ^= zobrist_flags[1 << b];
        }

        zobrist_flags[i] = x;
    }

    zobrist_flags[0] = 0; // if no flag changes, should be no change

    // Set random numbers for each square for enpassant
    for (size_t i = 0; i < std::size(zobrist_ep_buffer); ++i) {
        zobrist_ep_buffer[i] = rng();
    }
}

uint64_t Position::compute_zobrist() const {
    // Now we can begin forming the hash
    uint64_t hash = 0;

#ifdef ZOBRIST_INCLUDE_PIECES
    // Begin by going through each square and xoring into the hash
    uint64_t all_white = sides[WHITE].all();
    uint64_t all_black = sides[BLACK].all();

    for (int sq = 0; sq < 64; ++sq) {
        if (all_white & sq_to_bb(sq)) {
            hash ^= zobrist_piece[WHITE][piece_at[sq]][sq];
        } else if (all_black & sq_to_bb(sq)) {
            hash ^= zobrist_piece[BLACK][piece_at[sq]][sq];
        }
    }
#endif

#ifdef ZOBRIST_INCLUDE_FLAGS
    // xor with flags
    hash ^= zobrist_flags[flags];

#endif

#ifdef ZOBRIST_INCLUDE_SIDE
    // xor side if it is blacks turn to move
    if (to_move == BLACK) {
        hash ^= zobrist_side;
    }
#endif

#ifdef ZOBRIST_INCLUDE_EN_PASSANT_SQ
    hash ^= zobrist_ep(en_passant_sq);
#endif

    return hash;
}

uint64_t Position::zobrist_ep(int sq) const {
    return zobrist_ep_buffer[sq + 1];
};