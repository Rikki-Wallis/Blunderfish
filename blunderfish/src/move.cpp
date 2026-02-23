#include <array>

#include "blunderfish.h"
#include "generated_tables.h"

static constexpr std::array<std::array<uint32_t, 64>, 2> gen_rook_castle_flag_table() {
    std::array<std::array<uint32_t, 64>, 2> table{};

    table[WHITE][0] = POSITION_FLAG_WHITE_QCASTLE;
    table[WHITE][7] = POSITION_FLAG_WHITE_KCASTLE;

    table[BLACK][56] = POSITION_FLAG_BLACK_QCASTLE;
    table[BLACK][63] = POSITION_FLAG_BLACK_KCASTLE;

    return table;
}

static constexpr std::array<int, 64> gen_rook_jump_from_table() {
    std::array<int, 64> table;

    for (int i = 0; i < 64; ++i) {
        table[i] = i;
    }

    table[6]  = 7;
    table[2]  = 0;
    table[62] = 63;
    table[58] = 56;

    return table;
}

static std::array<int, 64> gen_rook_jump_to_table() {
    std::array<int, 64> table;

    for (int i = 0; i < 64; ++i) {
        table[i] = i;
    }

    table[6]  = 5;
    table[2]  = 3;
    table[62] = 61;
    table[58] = 59;

    return table;
}

static const std::array<std::array<uint32_t, 64>, 2> rook_castle_flag_table = gen_rook_castle_flag_table();
static const std::array<int, 64> rook_jump_from = gen_rook_jump_from_table();
static const std::array<int, 64> rook_jump_to = gen_rook_jump_to_table();

// King
static uint64_t king_moves(int from, uint64_t allies) {
    return king_move_table[from] & (~allies);
}

// Knights
static uint64_t knight_moves(int from, uint64_t allies) {
    return knight_move_table[from] & (~allies);
}

// Pawns
uint64_t white_pawn_single_push(uint64_t bb, uint64_t all_pieces) {
    uint64_t push  = (bb << 8) & (~all_pieces) & (~RANK_1);
    return push;
}

uint64_t white_pawn_double_push(uint64_t bb, uint64_t all_pieces) {
    bb &= RANK_2;

    uint64_t push  = (bb << 8) & (~all_pieces);
    uint64_t double_push = (push << 8) & (~all_pieces);

    return double_push;
}

uint64_t white_pawn_left_capture_no_mask(uint64_t bb) {
    uint64_t left  = (bb << 7) & (~FILE_H);
    return left;
}

uint64_t white_pawn_right_capture_no_mask(uint64_t bb) {
    uint64_t right = (bb << 9) & (~FILE_A);
    return right;
}

uint64_t black_pawn_single_push(uint64_t bb, uint64_t all_pieces) {
    uint64_t push  = (bb >> 8) & (~all_pieces) & (~RANK_8);
    return push;
}

uint64_t black_pawn_left_capture_no_mask(uint64_t bb) {
    uint64_t left  = (bb >> 9) & (~FILE_H);
    return left;
}

uint64_t black_pawn_right_capture_no_mask(uint64_t bb) {
    uint64_t right = (bb >> 7) & (~FILE_A);
    return right;
}

uint64_t black_pawn_double_push(uint64_t bb, uint64_t all_pieces) {
    bb &= RANK_7;
    uint64_t push  = (bb >> 8) & (~all_pieces);
    uint64_t double_push = (push >> 8) & (~all_pieces);
    return double_push;
}

// Magic Bitboards
static size_t magic_index(uint64_t all_pieces, uint64_t mask, uint64_t magic, size_t shift) {
    return static_cast<size_t>(((all_pieces & mask) * magic) >> shift);
}

uint64_t rook_moves(int from, uint64_t all_pieces, uint64_t allies) {
    size_t index = magic_index(all_pieces, rook_mask[from], rook_magic[from], rook_shift[from]);
    uint64_t moves = rook_move[from][index];
    return moves & (~allies);
}

uint64_t bishop_moves(int from, uint64_t all_pieces, uint64_t allies) {
    size_t index = magic_index(all_pieces, bishop_mask[from], bishop_magic[from], bishop_shift[from]);
    uint64_t moves = bishop_move[from][index];
    return moves & (~allies);
}

uint64_t queen_moves(int from, uint64_t all_pieces, uint64_t allies) {
    return bishop_moves(from, all_pieces, allies) | rook_moves(from, all_pieces, allies);
}

bool Position::is_king_square_attacked(int side, int square) const {
    int opp = opponent(side);

    uint64_t allies = sides[side].all() & ~(sides[side].bb[PIECE_KING]);
    uint64_t all = all_pieces() & ~(sides[side].bb[PIECE_KING]);

    uint64_t diag_mask = bishop_moves(square, all, allies);
    uint64_t straight_mask = rook_moves(square, all, allies);
    uint64_t knight_mask = knight_moves(square, allies);
    uint64_t pawn_mask = side == WHITE ? white_pawn_attacks_table[square] : black_pawn_attacks_table[square];
    uint64_t king_mask = king_moves(square, allies);

    if (diag_mask & sides[opp].bb[PIECE_BISHOP]) {
        return true;
    }

    if (straight_mask & sides[opp].bb[PIECE_ROOK]) {
        return true;
    }

    if (knight_mask & sides[opp].bb[PIECE_KNIGHT]) {
        return true;
    }

    if ((straight_mask | diag_mask) & sides[opp].bb[PIECE_QUEEN]) {
        return true;
    }

    if (king_mask & sides[opp].bb[PIECE_KING]) {
        return true;
    }

    if (pawn_mask & sides[opp].bb[PIECE_PAWN]) {
        return true;
    }

    return false;
}

template<typename T>
static T bool_to_mask(bool x) {
    return -(T)x;
}

bool Position::is_in_check(int colour) const {
    assert(std::popcount(sides[colour].bb[PIECE_KING]) == 1);
    int square = std::countr_zero(sides[colour].bb[PIECE_KING]);
    return is_king_square_attacked(colour, square);
}

std::span<Move> Position::generate_moves(std::span<Move> move_buf) const {
    size_t move_count = 0;

    auto new_move = [&](int from, int to, MoveType type, Piece end_piece) {
        assert("move buffer overflow" && move_count < move_buf.size());
        move_buf[move_count++] = encode_move(from, to, type, end_piece, to_move);
    };

    int opp = opponent(to_move);
    uint64_t opps = sides[opp].all();
    uint64_t allies = sides[to_move].all();
    uint64_t all = all_pieces();

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, allies))) {
            new_move(from, to, MOVE_NORMAL, PIECE_KING);
        }
    }

    // Castling

    uint32_t kcastle_flag = to_move == WHITE ? POSITION_FLAG_WHITE_KCASTLE : POSITION_FLAG_BLACK_KCASTLE;
    uint32_t qcastle_flag = to_move == WHITE ? POSITION_FLAG_WHITE_QCASTLE : POSITION_FLAG_BLACK_QCASTLE;

    bool can_kcastle = (flags & kcastle_flag) != 0;
    bool can_qcastle = (flags & qcastle_flag) != 0;

    if (is_in_check(to_move)) {
        can_kcastle = false;
        can_qcastle = false;
    }

    can_kcastle = can_kcastle && !is_king_square_attacked(to_move, to_move == WHITE ? 5 : 61);
    can_kcastle = can_kcastle && !is_king_square_attacked(to_move, to_move == WHITE ? 6 : 62);

    can_qcastle = can_qcastle && !is_king_square_attacked(to_move, to_move == WHITE ? 3 : 59);
    can_qcastle = can_qcastle && !is_king_square_attacked(to_move, to_move == WHITE ? 2 : 58);

    if (can_kcastle) {
        uint64_t castle_space = to_move == WHITE ? WHITE_SHORT_SPACING : BLACK_SHORT_SPACING;
        uint8_t from = to_move == WHITE ? 4 : 60;
        uint8_t to   = to_move == WHITE ? 6 : 62;
        assert(std::countr_zero(sides[to_move].bb[PIECE_KING]) == from);
        assert(sides[to_move].bb[PIECE_ROOK] & sq_to_bb(to_move == WHITE ? 7 : 63));
        if (!(castle_space & all)) {
            new_move(from, to, MOVE_SHORT_CASTLE, PIECE_KING);
        }
    }

    if (can_qcastle) {
        uint64_t castle_space = to_move == WHITE ? WHITE_LONG_SPACING : BLACK_LONG_SPACING;
        uint8_t from = to_move == WHITE ? 4 : 60;
        uint8_t to   = to_move == WHITE ? 2 : 58;
        assert(std::countr_zero(sides[to_move].bb[PIECE_KING]) == from);
        assert(sides[to_move].bb[PIECE_ROOK] & sq_to_bb(to_move == WHITE ? 0 : 56));
        if (!(castle_space & all)) {
            new_move(from, to, MOVE_LONG_CASTLE, PIECE_KING);
        }
    }

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KNIGHT])) {
        for (uint8_t to : set_bits(knight_moves(from, allies))) {
            new_move(from, to, MOVE_NORMAL, PIECE_KNIGHT);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_ROOK])) {
        for (uint8_t to : set_bits(rook_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL, PIECE_ROOK);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_BISHOP])) {
        for (uint8_t to : set_bits(bishop_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL, PIECE_BISHOP);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_QUEEN])) {
        for (uint8_t to : set_bits(queen_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL, PIECE_QUEEN);
        }
    }

    auto pawn_single_push           = to_move == WHITE ? white_pawn_single_push : black_pawn_single_push;
    auto pawn_double_push           = to_move == WHITE ? white_pawn_double_push : black_pawn_double_push;
    auto pawn_left_capture_no_mask  = to_move == WHITE ? white_pawn_left_capture_no_mask : black_pawn_left_capture_no_mask;
    auto pawn_right_capture_no_mask = to_move == WHITE ? white_pawn_right_capture_no_mask : black_pawn_right_capture_no_mask;

    uint64_t ep_mask                = en_passant_sq == NULL_SQUARE ? 0 : sq_to_bb(en_passant_sq);
    uint64_t promotion_rank         = to_move == WHITE ? RANK_8 : RANK_1;

    auto new_pawn_single_move = [&](int from, int to) {
        if (sq_to_bb(to) & promotion_rank) {
            new_move(from, to, MOVE_PROMOTION, PIECE_QUEEN);
            new_move(from, to, MOVE_PROMOTION, PIECE_KNIGHT);
            new_move(from, to, MOVE_PROMOTION, PIECE_BISHOP);
            new_move(from, to, MOVE_PROMOTION, PIECE_ROOK);
        } 
        else {
            new_move(from, to, MOVE_NORMAL, PIECE_PAWN);
        }
    };

    for (int to : set_bits(pawn_single_push(sides[to_move].bb[PIECE_PAWN], all))) {
        int offset[] = { -8, 8 };
        int from = to + offset[to_move];
        new_pawn_single_move(from, to);
    }

    for (int to : set_bits(pawn_double_push(sides[to_move].bb[PIECE_PAWN], all))) {
        int offset[] = { -16, 16 };
        int from = to + offset[to_move];
        new_move(from, to, MOVE_DOUBLE_PUSH, PIECE_PAWN);
    }

    for (int to : set_bits(pawn_left_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & opps)) {
        int offset[] = { -7, 9 };
        int from = to + offset[to_move];
        new_pawn_single_move(from, to);
    }

    for (int to : set_bits(pawn_right_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & opps)) {
        int offset[] = { -9, 7 };
        int from = to + offset[to_move];
        new_pawn_single_move(from, to);
    }

    for (int to : set_bits(pawn_left_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & ep_mask)) {
        int offset[] = { -7, 9 };
        int from = to + offset[to_move];
        new_move(from, to, MOVE_EN_PASSANT, PIECE_PAWN);
    }

    for (int to : set_bits(pawn_right_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & ep_mask)) {
        int offset[] = { -9, 7 };
        int from = to + offset[to_move];
        new_move(from, to, MOVE_EN_PASSANT, PIECE_PAWN);
    }

    return move_buf.subspan(0, move_count);
}

std::span<Move> Position::generate_captures(std::span<Move> move_buf) const {
    size_t move_count = 0;

    auto new_move = [&](int from, int to, MoveType type, Piece end_piece) {
        assert("move buffer overflow" && move_count < move_buf.size());
        move_buf[move_count++] = encode_move(from, to, type, end_piece, to_move);
    };

    int opp = opponent(to_move);
    uint64_t opps = sides[opp].all();
    uint64_t allies = sides[to_move].all();
    uint64_t all = all_pieces();

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, allies) & opps)) {
            new_move(from, to, MOVE_NORMAL, PIECE_KING);
        }
    }

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KNIGHT])) {
        for (uint8_t to : set_bits(knight_moves(from, allies) & opps)) {
            new_move(from, to, MOVE_NORMAL, PIECE_KNIGHT);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_ROOK])) {
        for (uint8_t to : set_bits(rook_moves(from, all, allies) & opps)) {
            new_move(from, to, MOVE_NORMAL, PIECE_ROOK);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_BISHOP])) {
        for (uint8_t to : set_bits(bishop_moves(from, all, allies) & opps)) {
            new_move(from, to, MOVE_NORMAL, PIECE_BISHOP);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_QUEEN])) {
        for (uint8_t to : set_bits(queen_moves(from, all, allies) & opps)) {
            new_move(from, to, MOVE_NORMAL, PIECE_QUEEN);
        }
    }

    auto pawn_left_capture_no_mask  = to_move == WHITE ? white_pawn_left_capture_no_mask : black_pawn_left_capture_no_mask;
    auto pawn_right_capture_no_mask = to_move == WHITE ? white_pawn_right_capture_no_mask : black_pawn_right_capture_no_mask;

    uint64_t ep_mask                = en_passant_sq == NULL_SQUARE ? 0 : sq_to_bb(en_passant_sq);
    uint64_t promotion_rank         = to_move == WHITE ? RANK_8 : RANK_1;

    auto new_pawn_single_move = [&](int from, int to) {
        if (sq_to_bb(to) & promotion_rank) {
            new_move(from, to, MOVE_PROMOTION, PIECE_QUEEN);
            new_move(from, to, MOVE_PROMOTION, PIECE_KNIGHT);
            new_move(from, to, MOVE_PROMOTION, PIECE_BISHOP);
            new_move(from, to, MOVE_PROMOTION, PIECE_ROOK);
        } 
        else {
            new_move(from, to, MOVE_NORMAL, PIECE_PAWN);
        }
    };

    for (int to : set_bits(pawn_left_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & opps)) {
        int offset[] = { -7, 9 };
        int from = to + offset[to_move];
        new_pawn_single_move(from, to);
    }

    for (int to : set_bits(pawn_right_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & opps)) {
        int offset[] = { -9, 7 };
        int from = to + offset[to_move];
        new_pawn_single_move(from, to);
    }

    for (int to : set_bits(pawn_left_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & ep_mask)) {
        int offset[] = { -7, 9 };
        int from = to + offset[to_move];
        new_move(from, to, MOVE_EN_PASSANT, PIECE_PAWN);
    }

    for (int to : set_bits(pawn_right_capture_no_mask(sides[to_move].bb[PIECE_PAWN]) & ep_mask)) {
        int offset[] = { -9, 7 };
        int from = to + offset[to_move];
        new_move(from, to, MOVE_EN_PASSANT, PIECE_PAWN);
    }

    return move_buf.subspan(0, move_count);
}

void Position::filter_moves(std::span<Move>& moves) {
    int side = to_move;

    for (int i = (int)moves.size()-1; i >= 0; --i) {
        bool illegal = false;

        make_move(moves[i]);

        if (is_in_check(side)) {
            illegal = true;
        }

        unmake_move(moves[i]);

        if (illegal) {
            moves[i] = moves.back();
            moves = moves.subspan(0, moves.size()-1);
        }
    }
}

std::unordered_map<std::string, size_t> Position::name_moves(std::span<Move> all) {
    std::vector<size_t> board[64];

    for (size_t i = 0; i < all.size(); ++i) {
        board[move_to(all[i])].push_back(i);
    }

    std::unordered_map<std::string, size_t> lookup;

    for (auto& moves : board) {
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = all[moves[i]];

            auto [f1, r1] = square_alg(move_from(m));

            bool is_capture = (piece_at[move_to(m)] != PIECE_NONE) || move_type(m) == MOVE_EN_PASSANT;
            bool need_file = piece_at[move_from(m)] == PIECE_PAWN && is_capture;
            bool need_rank = false;

            for (size_t j = 0; j < moves.size(); ++j) {
                if (i == j) {
                    continue;
                }

                Move n = all[moves[j]];

                if (piece_at[move_from(m)] != piece_at[move_from(n)]) {
                    continue;
                }

                auto [f2, r2] = square_alg(move_from(n));

                if (f1 != f2) {
                    need_file = true;
                }
                else if (r1 != r2) {
                    need_rank = true;
                }
            }

            auto [to_file, to_rank] = square_alg(move_to(m));

            std::string promotion_str = "";

            if (move_type(m) == MOVE_PROMOTION) {
                Piece piece = move_end_piece(m);
                promotion_str = std::format("={}", piece_alg_table[piece]);
            }

            std::string capture_str = is_capture ? "x" : "";
            std::string unambig_file = need_file ? std::format("{}", f1) : "";
            std::string unambig_rank = need_rank ? std::format("{}", r1) : "";

            std::string check_suffix = "";

            make_move(m);
            if (is_in_check(to_move)) {
                std::array<Move, 256> temp;
                auto moves = generate_moves(temp);
                filter_moves(moves);
                if (moves.size() == 0) {
                    check_suffix = "#";
                }
                else {
                    check_suffix = "+";
                }
            }
            unmake_move(m);

            std::string name = "";

            switch (move_type(m)) {
                case MOVE_SHORT_CASTLE:
                    name = "O-O";
                    break;
                case MOVE_LONG_CASTLE:
                    name = "O-O-O";
                    break;
                default:
                    name = std::format("{}{}{}{}{}{}{}{}", piece_alg_table[piece_at[move_from(m)]], unambig_file, unambig_rank, capture_str, to_file, to_rank, promotion_str, check_suffix);
                    break;
            }

            lookup.emplace(std::move(name), moves[i]);
        } 
    }

    return lookup;
}

// hopefully these functions get inlined and the rewrites get optimized away
inline void remove_piece(Position& pos, int side, Piece piece, size_t index) {
    assert(piece);
    assert((Piece)pos.piece_at[index] == piece);
    assert(pos.sides[side].bb[piece] & sq_to_bb(index));
    pos.sides[side].bb[piece] &= ~sq_to_bb(index);
    pos.piece_at[index] = PIECE_NONE;
}

inline void set_piece(Position& pos, int side, Piece piece, size_t index) {
    assert(piece);
    assert((Piece)pos.piece_at[index] == PIECE_NONE);
    pos.sides[side].bb[piece] |= sq_to_bb(index);
    pos.piece_at[index] = (uint8_t)piece;
}

int get_captured_square(Move move) {
    int captured_pos = move_to(move); // usually the piece being captured is at the square being moved to

    int offsets[] = {
        0, 0, -8, 8
    };

    int is_ep = move_type(move)==MOVE_EN_PASSANT;

    int side = move_side(move);
    captured_pos = move_to(move) + offsets[is_ep * 2 + side];

    return captured_pos;
}

bool Position::is_capture(Move mv) const {
    int captured_sq = get_captured_square(mv);
    Piece captured_piece = (Piece)piece_at[captured_sq];
    return captured_piece != PIECE_NONE;
}

void Position::make_move(Move move) {
    // First, check for a capture and remove the piece
    int captured_pos = get_captured_square(move);
    Piece captured_piece = (Piece)piece_at[captured_pos];
    assert(captured_piece != PIECE_KING);

    // Before we modify anything, record the destroyable data in the undo stack

    Undo undo = {
        .captured_piece = (uint8_t)captured_piece,
        .flags = flags,
        .en_passant_sq = en_passant_sq
    };

    assert(undo_count < MAX_DEPTH);
    undo_stack[undo_count++] = undo;

    // remove the captured piece

    uint64_t capture_mask = bool_to_mask<uint64_t>(captured_piece != PIECE_NONE) & sq_to_bb(captured_pos);
    sides[opponent(to_move)].bb[captured_piece] ^= capture_mask;
    piece_at[captured_pos] = PIECE_NONE; // if no captured piece, this is okay because it will be updated with the moving piece

    // move the piece

    Piece start_piece = (Piece)piece_at[move_from(move)];
    Piece end_piece = move_end_piece(move);

    uint64_t from_mask = sq_to_bb(move_from(move));
    uint64_t to_mask = sq_to_bb(move_to(move));

    sides[to_move].bb[start_piece] ^= from_mask;
    sides[to_move].bb[end_piece]   ^= to_mask;

    piece_at[move_from(move)] = PIECE_NONE;
    piece_at[move_to(move)] = end_piece;

    // Update castling rights when a rook or a king is moved, or a rook is taken

    uint32_t castle_flag_removal = 0;

    uint32_t flag_of_captured_rook = rook_castle_flag_table[opponent(to_move)][captured_pos];
    castle_flag_removal |= bool_to_mask<uint32_t>(captured_piece == PIECE_ROOK) & flag_of_captured_rook;

    uint32_t flag_of_king_move = to_move == WHITE ? (POSITION_FLAG_WHITE_QCASTLE | POSITION_FLAG_WHITE_KCASTLE) : (POSITION_FLAG_BLACK_QCASTLE | POSITION_FLAG_BLACK_KCASTLE);
    castle_flag_removal |= bool_to_mask<uint32_t>(start_piece == PIECE_KING) & flag_of_king_move; 

    uint32_t flag_of_rook_move = rook_castle_flag_table[to_move][move_from(move)];
    castle_flag_removal |= bool_to_mask<uint32_t>(start_piece == PIECE_ROOK) & flag_of_rook_move; 

    flags &= ~castle_flag_removal;

    // set the en passant square if a double push has occured

    int en_passant_table[] = {
        NULL_SQUARE, NULL_SQUARE, move_to(move) - 8, move_to(move) + 8
    };

    bool is_double_push = move_type(move) == MOVE_DOUBLE_PUSH;
    en_passant_sq = en_passant_table[is_double_push*2 + to_move];

    // move the rook when a castle is performed

    int rook_from = rook_jump_from[move_to(move)];
    int rook_to   = rook_jump_to  [move_to(move)];

    uint64_t rook_from_mask = sq_to_bb(rook_from);
    uint64_t rook_to_mask   = sq_to_bb(rook_to);
    uint64_t rook_jump_mask = bool_to_mask<uint64_t>(move_type(move) == MOVE_SHORT_CASTLE || move_type(move) == MOVE_LONG_CASTLE) & (rook_from_mask ^ rook_to_mask);

    sides[to_move].bb[PIECE_ROOK] ^= rook_jump_mask;

    uint8_t castle_mask = bool_to_mask<uint8_t>(move_type(move) == MOVE_SHORT_CASTLE || move_type(move) == MOVE_LONG_CASTLE);
    
    piece_at[rook_from] = (~castle_mask & piece_at[rook_from]) | (castle_mask & PIECE_NONE);
    piece_at[rook_to]   = (~castle_mask & piece_at[rook_to]  ) | (castle_mask & PIECE_ROOK);

    to_move = opponent(to_move);
}

void Position::unmake_move(Move move) {
    assert(undo_count > 0);
    Undo undo = undo_stack[--undo_count];

    to_move = opponent(to_move);

    // move the rook if a castle has to be undone

    int rook_from = rook_jump_from[move_to(move)];
    int rook_to   = rook_jump_to  [move_to(move)];

    uint64_t rook_from_mask = sq_to_bb(rook_from);
    uint64_t rook_to_mask   = sq_to_bb(rook_to);
    uint64_t rook_jump_mask = bool_to_mask<uint64_t>(move_type(move) == MOVE_SHORT_CASTLE || move_type(move) == MOVE_LONG_CASTLE) & (rook_from_mask ^ rook_to_mask);

    sides[to_move].bb[PIECE_ROOK] ^= rook_jump_mask;

    uint8_t castle_mask = bool_to_mask<uint8_t>(move_type(move) == MOVE_SHORT_CASTLE || move_type(move) == MOVE_LONG_CASTLE);
    
    piece_at[rook_to]   = (~castle_mask & piece_at[rook_to]  ) | (castle_mask & PIECE_NONE);
    piece_at[rook_from] = (~castle_mask & piece_at[rook_from]) | (castle_mask & PIECE_ROOK);

    // move the piece

    Piece end_piece = (Piece)piece_at[move_to(move)];
    Piece start_piece = move_type(move) == MOVE_PROMOTION ? PIECE_PAWN : end_piece;

    uint64_t to_mask   = sq_to_bb(move_to(move));
    uint64_t from_mask = sq_to_bb(move_from(move));

    sides[to_move].bb[end_piece] ^= to_mask;
    sides[to_move].bb[start_piece] ^= from_mask;

    piece_at[move_to(move)] = PIECE_NONE;
    piece_at[move_from(move)] = start_piece;

    // replace the captured piece

    int captured_square = get_captured_square(move);
    uint64_t captured_mask = bool_to_mask<uint64_t>(undo.captured_piece != PIECE_NONE) & sq_to_bb(captured_square);
    sides[opponent(to_move)].bb[undo.captured_piece] ^= captured_mask;
    piece_at[captured_square] = undo.captured_piece;

    flags = undo.flags;
    en_passant_sq = undo.en_passant_sq;
}

void Position::make_null_move() {
    to_move = opponent(to_move);
}

void Position::unmake_null_move(){
    to_move = opponent(to_move);
}