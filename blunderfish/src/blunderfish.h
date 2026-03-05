#pragma once

#include <optional>
#include <unordered_map>
#include <array>

#include "common.h"

#define ZOBRIST_INCLUDE_PIECES
#define ZOBRIST_INCLUDE_FLAGS
#define ZOBRIST_INCLUDE_EN_PASSANT_SQ
#define ZOBRIST_INCLUDE_SIDE

#define MAX_DEPTH 128

constexpr int64_t INF        = 400000000;
constexpr int64_t MATE_SCORE = 32000; // just shy of int16 bounds

static constexpr size_t TRANSPOSITION_TABLE_SIZE = 1 << 16;

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

enum Piece { // DO NOT CHANGE THE ORDER OF THIS
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

static const int32_t piece_value_table[NUM_PIECE_TYPES] = {
    0,
    100,
    500,
    300,
    300,
    900,
    0
};

using Move = uint32_t;

static constexpr Move NULL_MOVE = 0;

struct Side {
    uint64_t bb[NUM_PIECE_TYPES];

    uint64_t all() const;

    int64_t material_value() const;

    std::vector<uint64_t> get_bbs() const; 
};

static constexpr int NULL_SQUARE = -1;

struct Undo {
    uint32_t flags;
    int en_passant_sq;
    uint64_t zobrist;
    int64_t incremental_eval;
    bool is_checked[2];
};

struct TTEntry {
    uint16_t key16; // top 16 bits
    int16_t score;
    uint8_t depth;
    uint8_t flag;
    Move best_move;
};

struct TTCluster {
    TTEntry entries[4];
};

using KillerTable = std::array<std::array<Move, 2>, MAX_DEPTH>;
using HistoryTable = std::array<std::array<int32_t, 64>, NUM_PIECE_TYPES>;

class TranspositionTable {
public:
    std::vector<TTCluster> table;

    TranspositionTable()
        : table(TRANSPOSITION_TABLE_SIZE) {}

    TTCluster& operator[](uint64_t index) {
        return table[index];
    }
};

struct ZobristTable {
    uint64_t side;
    std::array<uint64_t, 64> piece[2][NUM_PIECE_TYPES];
    uint64_t flags[255];
    uint64_t ep_table[65];

    uint64_t ep(int sq) const {
        return ep_table[sq + 1];
    }
};

extern const ZobristTable zobrist_table;

struct EvalParameters {
    // Material
    int piece_values[NUM_PIECE_TYPES];

    // PSTs
    int mg_pst[NUM_PIECE_TYPES][64];
    int eg_pst[NUM_PIECE_TYPES][64];

    // King Safety
    int q_castling_bonus;
    int q_castling_penalty;
    int k_castling_bonus;
    int k_castling_penalty;
    int shelter_strength_bonus;
    int shelter_strength_penalty;
    int open_file_bonus;
    int open_file_penalty;

    // Imbalance
    int bishop_pair_bonus;

    // Pawn Structure
    int isloated_pawn_penalty;
    int doubled_pawn_penalty;
    int connected_pawn_bonus;
    int passed_pawn_bonus;
};

struct Position {
    Side sides[2];
    int to_move;
    int en_passant_sq;
    uint8_t piece_at[64];
    uint32_t flags;
    int64_t incremental_eval;

    bool is_checked[2];

    uint64_t zobrist;

    // benchmarking statistics
    int node_count;
    int qnode_count;
    int beta_cutoffs;
    int null_prunes; 
    int cutoff_index_count;
    int cutoff_index_sum;

    std::array<Undo, MAX_DEPTH> undo_stack;
    int undo_count;

    Position()
        : to_move(WHITE), en_passant_sq(NULL_SQUARE), flags(0), undo_count(0)
    {
        memset(sides, 0, sizeof(sides));
        memset(piece_at, 0, sizeof(piece_at));
        memset(undo_stack.data(), 0, sizeof(MAX_DEPTH * sizeof(Undo)));
        zobrist = compute_zobrist();
        reset_benchmarking_statistics();
    }

    Position(Position&&) = default;
    Position(const Position&) = delete;
    const Position& operator=(const Position&) = delete;

    void display(bool display_metadata=false) const;
    static std::optional<Position> decode_fen_string(const std::string& fen);

    std::span<Move> generate_moves(std::span<Move> move_buf) const;
    std::span<Move> generate_captures(std::span<Move> move_buf) const;

    std::unordered_map<std::string, Move> name_moves(std::span<Move> moves);

    uint64_t all_pieces() const;

    void make_move(Move move);
    void unmake_move(Move move);

    void make_null_move();
    void unmake_null_move();

    std::vector<Side> get_sides() const;  
    
    int get_king_sq(int side) const;
    uint64_t generate_pin_mask(int side) const;

    bool is_king_square_attacked(int side, int square) const;

    int lowest_value_defender(int defender_side, int sq, uint64_t occupancy) const;
    int see(Move m) const;

    void filter_moves(std::span<Move>& moves);

    void verify_integrity() const;

    int64_t compute_eval() const;
    int64_t signed_eval() const;

    // @note if no castle, make rook_from == rook_ro
    void update_eval(Piece captured_piece, int captured_pos, Piece moving_piece_start, Piece moving_piece_end, int move_from, int move_to, int rook_from, int rook_to, int side);
    void update_is_checked();

    int64_t pruned_negamax(int depth, TranspositionTable& tt, HistoryTable& history, KillerTable& killers, int ply, bool allow_null, int64_t alpha, int64_t beta);
    int64_t quiescence(int ply, int64_t alpha, int64_t beta);

    int32_t mvv_lva_score(Move mv, int32_t offset) const;

    std::pair<Move, int64_t> best_move_internal(std::span<Move> moves, int depth, TranspositionTable& tt, Move last_best_move, HistoryTable& history, KillerTable& killers, int64_t alpha, int64_t beta);
    Move best_move(std::span<Move> moves, int depth);

    uint64_t compute_zobrist() const;

    void update_en_passant_sq(int sq);

    int64_t total_non_pawn_value() const; // used for null move reduction heuristic

    void reset_benchmarking_statistics();

    // eval
    int64_t pawn_structure(int colour, uint64_t ally_pawn_bb) const;
    int64_t king_safety(int colour, uint64_t king_bb, uint64_t pawn_bb) const;
    int64_t bishop_imbalance() const;
};

int get_captured_square(int to, MoveType ty, int side);

int32_t mg_unsigned_pst_value(Piece piece, int square, int side);
int32_t eg_unsigned_pst_value(Piece piece, int square, int side);

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

inline Piece move_captured_piece(Move move) {
    return Piece((move >> 19) & 0b111);
}

inline bool is_capture(Move move) {
    return move_captured_piece(move) != PIECE_NONE;
}

inline int move_captured_square(Move move) {
    return get_captured_square(move_to(move), move_type(move), move_side(move));
}

inline Move encode_move(int from, int to, MoveType type, Piece end_piece, int side, Piece captured) {
    Move mv = (uint32_t)from & 0b111111; 
    mv     |= ((uint32_t)to & 0b111111) << 6;
    mv     |= ((uint32_t)type & 0b111) << 12;
    mv     |= ((uint32_t)end_piece & 0b111) << 15;
    mv     |= ((uint32_t)side & 1) << 18;
    mv     |= ((uint32_t)captured & 0b111) << 19;
    return mv;
}

inline uint64_t bb_to_file(uint64_t bb) {
    static const uint64_t file_table[8] = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
    if (bb == 0) return 0;

    unsigned long index;
    _BitScanForward64(&index, bb); 

    int file = index % 8; 
    return file_table[file];
}

template<typename T>
static T bool_to_mask(bool x) {
    return static_cast<T>(-static_cast<int>(x));
}