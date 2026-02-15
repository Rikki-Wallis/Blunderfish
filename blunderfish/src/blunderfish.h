#pragma once

#include <optional>
#include <unordered_map>

#include "common.h"

#define MAX_DEPTH 32

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

static constexpr uint64_t WHITE_SHORT_SPACING = (FILE_F | FILE_G) & RANK_1;
static constexpr uint64_t WHITE_LONG_SPACING  = (FILE_B | FILE_C | FILE_D) & RANK_1;
static constexpr uint64_t BLACK_SHORT_SPACING = (FILE_F | FILE_G) & RANK_8;
static constexpr uint64_t BLACK_LONG_SPACING  = (FILE_B | FILE_C | FILE_D) & RANK_8;

enum Colour : uint8_t {
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

enum MoveType {
    MOVE_NORMAL,
    MOVE_DOUBLE_PUSH,
    MOVE_EN_PASSANT,
    MOVE_SHORT_CASTLE,
    MOVE_LONG_CASTLE,
    MOVE_PROMOTE_QUEEN,
    MOVE_PROMOTE_BISHOP,
    MOVE_PROMOTE_ROOK,
    MOVE_PROMOTE_KNIGHT,
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

using Move = uint16_t;

struct Side {
    uint64_t bb[NUM_PIECE_TYPES];
    int flags;

    void set_can_castle_kingside(bool);
    void set_can_castle_queenside(bool);

    bool can_castle_kingside() const;
    bool can_castle_queenside() const;

    uint64_t all() const;

    int64_t material_value() const;

    std::vector<uint64_t> get_bbs() const; 
};

static constexpr int NULL_SQUARE = 0xffffffff;

struct Undo {
    uint8_t captured_piece;
    int flags[2];
    int en_passant_sq;
};

struct Position {
    Side sides[2];
    int to_move;
    int en_passant_sq;
    uint8_t piece_at[64];

    Undo undo_stack[MAX_DEPTH];
    int undo_count;

    Position()
        : to_move(WHITE), en_passant_sq(NULL_SQUARE), undo_count(0)
    {
        memset(sides, 0, sizeof(sides));
        memset(piece_at, 0, sizeof(piece_at));
        memset(undo_stack, 0, sizeof(undo_stack));
    }

    Position(Position&&) = default;
    Position(const Position&) = delete;
    const Position& operator=(const Position&) = delete;

    void display(bool display_metadata=false) const;
    static std::optional<Position> decode_fen_string(const std::string& fen);

    std::span<Move> generate_moves(std::span<Move> move_buf) const;

    std::unordered_map<std::string, size_t> name_moves(std::span<Move> moves) const;

    uint64_t all_pieces() const;

    void make_move(const Move& move);
    void unmake_move(const Move& move);

    std::vector<Side> get_sides() const;  

    bool is_in_check(int colour) const;
    // omits king
    bool is_attacked(int side, int square) const;

    void filter_moves(std::span<Move>& moves); 

    void verify_integrity() const;

    int64_t eval() const;
    int64_t negamax(int depth, int ply);

    int best_move(std::span<Move> moves, uint8_t depth);
};

inline std::pair<char, int> square_alg(size_t sq) {
    char file = sq % 8 + 'a';
    int rank  = int(sq / 8 + 1);

    return {file, rank};
}

inline int opponent(int side) {
    return (side + 1) & 1;
}

inline uint64_t sq_to_bb(size_t index) {
    return (uint64_t)1 << index;
}

uint64_t perft_search(int depth, Position& position);

inline int move_from(Move move) {
    return move & 0b111111;
}

inline int move_to(Move move) {
    return (move >> 6) & 0b111111;
}

inline MoveType move_type(Move move) {
    int type = (move >> 12) & 0b1111;
    return (MoveType)type;
}

inline Move encode_move(int from, int to, MoveType type) {
    uint16_t mv = (uint16_t)from; 
    mv         |= ((uint16_t)to) << 6;
    mv         |= ((uint16_t)type) << 12;
    return mv;
}