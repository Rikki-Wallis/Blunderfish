#pragma once

#include <optional>
#include <unordered_map>

#include "common.h"

static constexpr uint64_t RANK_1 = 0x00000000000000ff;
static constexpr uint64_t RANK_2 = 0x000000000000ff00;
static constexpr uint64_t RANK_3 = 0x0000000000ff0000;
static constexpr uint64_t RANK_4 = 0x00000000ff000000;
static constexpr uint64_t RANK_5 = 0x000000ff00000000;
static constexpr uint64_t RANK_6 = 0x0000ff0000000000;
static constexpr uint64_t RANK_7 = 0x00ff000000000000;
static constexpr uint64_t RANK_8 = 0xff00000000000000;

static constexpr uint64_t FILE_A = 0x0101010101010101;
static constexpr uint64_t FILE_B = 0x0202020202020202;
static constexpr uint64_t FILE_C = 0x0404040404040404;
static constexpr uint64_t FILE_D = 0x0808080808080808;
static constexpr uint64_t FILE_E = 0x1010101010101010;
static constexpr uint64_t FILE_F = 0x2020202020202020;
static constexpr uint64_t FILE_G = 0x4040404040404040;
static constexpr uint64_t FILE_H = 0x8080808080808080;

enum {
    WHITE,
    BLACK
};

enum Piece {
    PIECE_NONE,
    PIECE_PAWN,
    PIECE_ROOK,
    PIECE_KNIGHT,
    PIECE_BISHOP,
    PIECE_QUEEN,
    PIECE_KING,
    NUM_PIECE_TYPES
};

static const char* piece_alg_table[NUM_PIECE_TYPES] = {
    "uninitialized",
    "",
    "R",
    "N",
    "B",
    "Q",
    "K",
};

struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piece;
    uint8_t flags;
};

struct Side {
    uint64_t bb[NUM_PIECE_TYPES];
    int flags;

    void set_can_castle_kingside(bool);
    void set_can_castle_queenside(bool);

    bool can_castle_kingside() const;
    bool can_castle_queenside() const;

    uint64_t all() const;
};

static constexpr int NULL_SQUARE = 0xffffffff;

struct Position {
    Side sides[2];
    int to_move;
    int en_passant_sq = NULL_SQUARE;
    uint8_t piece_at[64];

    void display(bool display_metadata=false) const;
    static std::optional<Position> decode_fen_string(const std::string& fen);

    std::span<Move> generate_moves(std::span<Move> move_buf) const;

    std::unordered_map<std::string, size_t> name_moves(std::span<Move> moves) const;

    uint64_t all_pieces() const;

    Position execute_move(const Move& move) const;
};

inline std::pair<char, int> square_alg(size_t sq) {
    char file = sq % 8 + 'a';
    int rank  = sq / 8 + 1;

    return {file, rank};
}

inline int opponent(int side) {
    return (side + 1) & 1;
}

inline uint64_t sq_to_bb(uint8_t index) {
    return (uint64_t)1 << index;
}