#include <cstdio>
#include <vector>
#include <cstdint>
#include <random>
#include "blunderfish.h"

void Position::initialise_zobrist() {
    /** 
        VERY IMPORTANT TO ONLY RUN AT START OF PROGRAM.
        This method sets up all of the piece tables and
        initialises the zobrist hash and thus is very slow.
    */
    std::vector<uint8_t> pieces = {PIECE_PAWN, PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN, PIECE_KING};
    std::vector<uint8_t> sides_loop = {WHITE, BLACK};

    // Create and seed rand num gen
    std::mt19937_64 rng(8008135);
    std::uniform_int_distribution<uint64_t> dist(std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());

    // Set random numbers for each square for each piece
    for (uint8_t side : sides_loop) {
        for (uint8_t piece : pieces) {
            for (int i=0; i<64; ++i) {
                zobrist_piece[side][piece][i] = rng();
            }
        }
    }      

    // Set random number for black to move
    zobrist_side = rng();

    // Set random numbers for castling
    std::vector<uint8_t> castle_flags = {POSITION_FLAG_WHITE_QCASTLE, POSITION_FLAG_WHITE_KCASTLE, POSITION_FLAG_BLACK_QCASTLE, POSITION_FLAG_BLACK_KCASTLE};
    for (uint8_t flag : castle_flags) {
        zobrist_castling[flag] = rng();
    }

    // Set random numbers for each row for enpassant
    for (int i=0; i<8; ++i) {
        zobrist_ep[i] = rng();
    }

    // Now we can begin forming the hash
    zobrist = 0;

    // Begin by going through each square and xoring into the hash
    uint64_t all_white = sides[WHITE].all();
    uint64_t all_black = sides[BLACK].all();
    for (int i=0; i<64; ++i) {
        if (all_white & sq_to_bb(i)) {
            zobrist ^= zobrist_piece[WHITE][piece_at[i]][i];
        } else if (all_black & sq_to_bb(i)) {
            zobrist ^= zobrist_piece[BLACK][piece_at[i]][i];
        }
    }

    // xor with castling permissions
    if (flags & POSITION_FLAG_WHITE_KCASTLE) {
        zobrist ^= zobrist_castling[POSITION_FLAG_WHITE_KCASTLE];
    }
    if (flags & POSITION_FLAG_WHITE_QCASTLE) {
        zobrist ^= zobrist_castling[POSITION_FLAG_WHITE_QCASTLE];
    }
    if (flags & POSITION_FLAG_BLACK_KCASTLE) {
        zobrist ^= zobrist_castling[POSITION_FLAG_BLACK_KCASTLE];
    }
    if (flags & POSITION_FLAG_BLACK_QCASTLE) {
        zobrist ^= zobrist_castling[POSITION_FLAG_BLACK_QCASTLE];
    }

    // Only xor side if it is blacks turn to move
    if (to_move == BLACK) {
        zobrist ^= zobrist_side;
    }

    // If enpassant present xor aswell
    if (en_passant_sq) {
        zobrist ^= zobrist_ep[en_passant_sq/8];
    }
}

// void Position::update_zobrist(Move& move) {
// }