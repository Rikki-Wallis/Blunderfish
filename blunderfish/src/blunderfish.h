#pragma once

#include <optional>
#include <unordered_map>
#include <array>

#include "common.h"

#define MAX_DEPTH 64

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
    MOVE_PROMOTION,
};

enum PositionFlags {
    POSITION_FLAG_NONE = 0,
    POSITION_FLAG_WHITE_QCASTLE = (1 << 0),
    POSITION_FLAG_WHITE_KCASTLE = (1 << 1),
    POSITION_FLAG_BLACK_QCASTLE = (1 << 2),
    POSITION_FLAG_BLACK_KCASTLE = (1 << 3),
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

using Move = uint32_t;

static constexpr Move NULL_MOVE = 0;

struct Side {
    uint64_t bb[NUM_PIECE_TYPES];

    uint64_t all() const;

    int64_t material_value() const;

    std::vector<uint64_t> get_bbs() const; 
};

static constexpr int NULL_SQUARE = 0xffffffff;

struct Undo {
    uint8_t captured_piece;
    uint32_t flags;
    int en_passant_sq;
    uint64_t zobrist;
};

using KillerTable = std::array<std::array<Move, 2>, MAX_DEPTH>;
using HistoryTable = std::array<std::array<int32_t, 64>, NUM_PIECE_TYPES>;

struct Position {
    Side sides[2];
    int to_move;
    int en_passant_sq;
    uint8_t piece_at[64];
    uint32_t flags;

    // Zobrist
    uint64_t zobrist;
    uint64_t zobrist_side;
    std::unordered_map<int, std::unordered_map<int, std::array<uint64_t, 64>>> zobrist_piece;
    std::array<uint64_t, 4> zobrist_castling;
    std::array<uint64_t, 8> zobrist_ep;

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
    std::span<Move> generate_captures(std::span<Move> move_buf) const;

    std::unordered_map<std::string, size_t> name_moves(std::span<Move> moves);

    uint64_t all_pieces() const;

    void make_move(Move move);
    void unmake_move(Move move);

    std::vector<Side> get_sides() const;  

    bool is_in_check(int colour) const;
    bool is_king_square_attacked(int side, int square) const;

    void filter_moves(std::span<Move>& moves);

    void verify_integrity() const;

    int64_t eval() const;
    int64_t negamax(int depth, int ply);
    int64_t pruned_negamax(int depth, HistoryTable& history, KillerTable& killers, int ply, int64_t alpha, int64_t beta);
    int64_t quiescence(int ply, int64_t alpha, int64_t beta);

    int32_t mvv_lva_score(Move mv) const;

    bool is_capture(Move mv) const;

    Move best_move_internal(std::span<Move> moves, int depth, Move last_best_move, HistoryTable& history, KillerTable& killers);
    Move best_move(std::span<Move> moves, int depth);

    void initialise_zobrist();
    void update_zobrist(Move& move);
};

int get_captured_square(Move move);
int32_t piece_value_centipawns(Piece piece);

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
    int type = (move >> 12) & 0b111;
    return (MoveType)type;
}

inline Piece move_end_piece(Move move) {
    int piece = (move >> 15) & 0b111;
    return (Piece)piece;
}

inline int move_side(Move move) {
    int side = (move >> 18) & 1;
    return side;
}

inline Move encode_move(int from, int to, MoveType type, Piece end_piece, int side) {
    Move mv = (uint32_t)from; 
    mv     |= ((uint32_t)to) << 6;
    mv     |= ((uint32_t)type) << 12;
    mv     |= ((uint32_t)end_piece) << 15;
    mv     |= ((uint32_t)side & 1) << 18;
    return mv;
}