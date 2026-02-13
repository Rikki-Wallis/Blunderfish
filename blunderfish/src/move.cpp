#include "blunderfish.h"
#include "generated_tables.h"

// King
static uint64_t king_moves(uint8_t from, uint64_t allies) {
    uint64_t bb = sq_to_bb(from);

    uint64_t l  = (bb >> 1) & (~FILE_H);
    uint64_t lu = (bb << 7) & (~FILE_H);
    uint64_t u  = (bb << 8);
    uint64_t ru = (bb << 9) & (~FILE_A);
    uint64_t r  = (bb << 1) & (~FILE_A);
    uint64_t rd = (bb >> 7) & (~FILE_A);
    uint64_t d  = (bb >> 8);
    uint64_t ld = (bb >> 9) & (~FILE_H);

    uint64_t m = l | lu | u | ru | r | rd | d | ld;

    return m & (~allies);
}

// Knights
static uint64_t knight_moves(int from, uint64_t allies) {
    uint64_t bb = sq_to_bb(from);

    uint64_t ul = (bb >> 17) & ~FILE_H;
    uint64_t ur = (bb >> 15) & ~FILE_A; 
    uint64_t lu = (bb >> 10) & ~(FILE_H | FILE_G); 
    uint64_t ru = (bb >> 6)  & ~(FILE_A | FILE_B); 
    uint64_t dl = (bb << 17) & ~FILE_A; 
    uint64_t dr = (bb << 15) & ~FILE_H; 
    uint64_t ld = (bb << 10) & ~(FILE_A | FILE_B); 
    uint64_t rd = (bb << 6)  & ~(FILE_H | FILE_G); 

    return (ul | ur | lu | ru | dl | dr | ld | rd) & ~(allies);
}

// Pawns
uint64_t white_pawn_single_moves(uint8_t from, uint64_t opps, uint64_t all_pieces) {
    uint64_t bb = sq_to_bb(from);

    uint64_t push  = (bb << 8) & (~all_pieces) & (~RANK_1);
    uint64_t left  = (bb << 7) & (~FILE_H) & opps;
    uint64_t right = (bb << 9) & (~FILE_A) & opps;

    return push | left | right;
}

uint64_t black_pawn_single_moves(uint8_t from, uint64_t opps, uint64_t all_pieces) {
    uint64_t bb = sq_to_bb(from);

    uint64_t push  = (bb >> 8) & (~all_pieces) & (~RANK_8);
    uint64_t left  = (bb >> 9) & (~FILE_H) & opps;
    uint64_t right = (bb >> 7) & (~FILE_A) & opps;

    return push | left | right;
}

uint64_t white_pawn_attacks(uint64_t pawns_bb) {
    uint64_t left  = (pawns_bb << 7) & (~FILE_H);
    uint64_t right = (pawns_bb << 9) & (~FILE_A);
    return left | right;
}

uint64_t black_pawn_attacks(uint64_t pawns_bb) {
    uint64_t left  = (pawns_bb >> 9) & (~FILE_H);
    uint64_t right = (pawns_bb >> 7) & (~FILE_A);
    return left | right;
}

uint64_t white_pawn_double_moves(uint8_t from, uint64_t all_pieces) {
    uint64_t bb = sq_to_bb(from) & RANK_2;

    uint64_t push  = (bb << 8) & (~all_pieces);
    uint64_t double_push = (push << 8) & (~all_pieces);

    return double_push;
}

uint64_t black_pawn_double_moves(uint8_t from, uint64_t all_pieces) {
    uint64_t bb = sq_to_bb(from) & RANK_7;

    uint64_t push  = (bb >> 8) & (~all_pieces);
    uint64_t double_push = (push >> 8) & (~all_pieces);

    return double_push;
}

uint64_t white_pawn_enpassant_moves(uint8_t from, int enpassant_sq) {
    uint64_t mask = enpassant_sq == NULL_SQUARE ? 0 : sq_to_bb(enpassant_sq);
    uint64_t bb = sq_to_bb(from);

    uint64_t left_capture  = (bb << 7) & (~FILE_H) & mask;
    uint64_t right_capture = (bb << 9) & (~FILE_A) & mask;

    return left_capture | right_capture;
}

uint64_t black_pawn_enpassant_moves(uint8_t from, int enpassant_sq) {
    uint64_t mask = enpassant_sq == NULL_SQUARE ? 0 : sq_to_bb(enpassant_sq);
    uint64_t bb = sq_to_bb(from);

    uint64_t left_capture  = (bb >> 9) & (~FILE_H) & mask;
    uint64_t right_capture = (bb >> 7) & (~FILE_A) & mask;

    return left_capture | right_capture;
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

bool Position::is_in_check(int colour) const {
    uint64_t king_bb = sides[colour].bb[PIECE_KING];
    assert(std::popcount(king_bb) == 1);
    int king_index = std::countr_zero(king_bb);

    int opp = opponent(colour);

    uint64_t all = all_pieces();
    uint64_t allies = sides[colour].all();
    uint64_t opps = sides[opp].all();

    uint64_t diags = bishop_moves(king_index, all, allies);
    uint64_t straights = rook_moves(king_index, all, allies);
    uint64_t ls = knight_moves(king_index, allies);

    for (uint8_t from : set_bits(sides[opp].bb[PIECE_KNIGHT] & ls)) {
        if (knight_moves(from, opps) & king_bb) {
            return true;
        }
    }

    for (uint8_t from : set_bits(sides[opp].bb[PIECE_ROOK] & straights)) {
        if(rook_moves(from, all, opps) & king_bb) {
            return true;
        }
    }

    for (uint8_t from : set_bits(sides[opp].bb[PIECE_BISHOP] & diags)) {
        if(bishop_moves(from, all, opps) & king_bb) {
            return true;
        }
    }

    for (uint8_t from : set_bits(sides[opp].bb[PIECE_QUEEN] & (diags | straights))) {
        if(queen_moves(from, all, opps) & king_bb) {
            return true;
        }
    }

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING] & (diags | straights))) {
        if(king_moves(from, opps)) {
            return true;
        }
    }

    auto pawn_single_moves = opp == WHITE ? white_pawn_single_moves : black_pawn_single_moves;

    for (uint8_t from : set_bits(sides[opp].bb[PIECE_PAWN] & diags)) {
        if(pawn_single_moves(from, allies, all) & king_bb) {
            return true;
        }
    }

    return false;
}


std::span<Move> Position::generate_moves(std::span<Move> move_buf) const {
    size_t move_count = 0;

    auto new_move = [&](int from, int to, MoveType type) {
        assert("move buffer overflow" && move_count < move_buf.size());
        move_buf[move_count++] = encode_move(from, to, type);
    };

    int opp = opponent(to_move);
    uint64_t opps = sides[opp].all();
    uint64_t allies = sides[to_move].all();
    uint64_t all = all_pieces();

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, allies))) {
            new_move(from, to, MOVE_NORMAL);
        }
    }

    // Castling
    if (sides[to_move].can_castle_kingside()) {
        uint64_t castle_space = to_move == WHITE ? WHITE_SHORT_SPACING : BLACK_SHORT_SPACING;
        uint8_t from = to_move == WHITE ? 4 : 60;
        uint8_t to   = to_move == WHITE ? 6 : 62;
        if (!(castle_space & all)) {
            new_move(from, to, MOVE_SHORT_CASTLE);
        }
    }

    if (sides[to_move].can_castle_queenside()) {
        uint64_t castle_space = to_move == WHITE ? WHITE_LONG_SPACING : BLACK_LONG_SPACING;
        uint8_t from = to_move == WHITE ? 4 : 60;
        uint8_t to   = to_move == WHITE ? 2 : 58;
        if (!(castle_space & all)) {
            new_move(from, to, MOVE_LONG_CASTLE);
        }
    }

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KNIGHT])) {
        for (uint8_t to : set_bits(knight_moves(from, allies))) {
            new_move(from, to, MOVE_NORMAL);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_ROOK])) {
        for (uint8_t to : set_bits(rook_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_BISHOP])) {
        for (uint8_t to : set_bits(bishop_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_QUEEN])) {
        for (uint8_t to : set_bits(queen_moves(from, all, allies))) {
            new_move(from, to, MOVE_NORMAL);
        }
    }

    auto pawn_single_moves    = to_move == WHITE ? white_pawn_single_moves : black_pawn_single_moves;
    auto pawn_double_moves    = to_move == WHITE ? white_pawn_double_moves : black_pawn_double_moves;
    auto pawn_enpassant_moves = to_move == WHITE ? white_pawn_enpassant_moves : black_pawn_enpassant_moves;
    auto promotion_rank       = to_move == WHITE ? RANK_8 : RANK_1;

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_PAWN])) {
        for (uint8_t to : set_bits(pawn_single_moves(from, opps, all))) {
            
            if (sq_to_bb(to) & promotion_rank) {
                new_move(from, to, MOVE_PROMOTE_KNIGHT);
                new_move(from, to, MOVE_PROMOTE_BISHOP);
                new_move(from, to, MOVE_PROMOTE_ROOK);
                new_move(from, to, MOVE_PROMOTE_QUEEN);
            } 
            else {
                new_move(from, to, MOVE_NORMAL);
            }
        }

        for (uint8_t to : set_bits(pawn_double_moves(from, all))) {
            new_move(from, to, MOVE_DOUBLE_PUSH);
        }

        for (uint8_t to : set_bits(pawn_enpassant_moves(from, en_passant_sq))) {
            new_move(from, to, MOVE_EN_PASSANT);
        }
    }

    return move_buf.subspan(0, move_count);
}

void Position::filter_moves(std::span<Move>& moves) {
    int side = to_move; // ALERT! do NOT use to_move because it is altered by make_move

    bool checked = is_in_check(to_move);
 
    for (int i = (int)moves.size()-1; i >= 0; --i) {
        Move m = moves[i];

        bool is_castle = move_type(m) == MOVE_LONG_CASTLE || move_type(m) == MOVE_SHORT_CASTLE;
        bool illegal = is_castle && checked;

        make_move(m);

        if (is_in_check(side)) {
            illegal = true;
        }

        // Make sure cant move through check when castling
        if (is_castle) {
            uint64_t possible_check_pos = 0;

            if (move_type(m) == MOVE_SHORT_CASTLE) {
                possible_check_pos = side == WHITE ? WHITE_SHORT_SPACING : BLACK_SHORT_SPACING;
            } else {
                possible_check_pos = side == WHITE ? WHITE_LONG_SPACING : BLACK_LONG_SPACING;
            }

            if (possible_check_pos & generate_attacks(opponent(side))) {
                illegal = true;
            }
        }

        unmake_move(m);

        if (illegal) {
            moves[i] = moves.back();
            moves = moves.first(moves.size() - 1);
        }
    }
}

std::unordered_map<std::string, size_t> Position::name_moves(std::span<Move> all) const {
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

                Move& n = all[moves[j]];

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
            switch (move_type(m)) {
                case MOVE_PROMOTE_KNIGHT:
                    promotion_str = "=N";
                    break;
                case MOVE_PROMOTE_BISHOP:
                    promotion_str = "=B";
                    break;
                case MOVE_PROMOTE_QUEEN:
                    promotion_str = "=Q";
                    break;
                case MOVE_PROMOTE_ROOK:
                    promotion_str = "=R";
                    break;
            }

            std::string capture_str = is_capture ? "x" : "";
            std::string unambig_file = need_file ? std::format("{}", f1) : "";
            std::string unambig_rank = need_rank ? std::format("{}", r1) : "";

            std::string name = "";

            switch (move_type(m)) {
                case MOVE_SHORT_CASTLE:
                    name = "O-O";
                    break;
                case MOVE_LONG_CASTLE:
                    name = "O-O-O";
                    break;
                default:
                    name = std::format("{}{}{}{}{}{}{}", piece_alg_table[piece_at[move_from(m)]], unambig_file, unambig_rank, capture_str, to_file, to_rank, promotion_str);
                    break;
            }

            lookup.emplace(std::move(name), moves[i]);
        } 
    }

    return lookup;
}

// hopefully these functions get inlined and the rewrites get optimized away
inline void remove_piece(Position& pos, int side, Piece piece, size_t index) {
    assert((Piece)pos.piece_at[index] == piece);
    assert(pos.sides[side].bb[piece] & sq_to_bb(index));
    pos.sides[side].bb[piece] &= ~sq_to_bb(index);
    pos.piece_at[index] = PIECE_NONE;
}

inline void set_piece(Position& pos, int side, Piece piece, size_t index) {
    assert((Piece)pos.piece_at[index] == PIECE_NONE);
    pos.sides[side].bb[piece] |= sq_to_bb(index);
    pos.piece_at[index] = (uint8_t)piece;
}

static int get_captured_square(const Move& move, int to_move) {
    int captured_pos = move_to(move); // usually the piece being captured is at the square being moved to

    if (move_type(move) == MOVE_EN_PASSANT) { // en passant is the exception
        int offsets[] = { -8, 8 };
        captured_pos = move_to(move) + offsets[to_move];
    }

    return captured_pos;
}

static std::pair<int, int> short_castle_start_and_end_rook_squares(int side) {
    int old_pos[] = { 7, 63 };
    int new_pos[] = { 5, 61 };
    return {old_pos[side], new_pos[side]};
}

static std::pair<int, int> long_castle_start_and_end_rook_squares(int side) {
    int old_pos[] = { 0, 56 };
    int new_pos[] = { 3, 59 };
    return {old_pos[side], new_pos[side]};
}

void Position::make_move(const Move& move) {
    // First, check for a capture and remove the piece

    int captured_pos = get_captured_square(move, to_move);
    Piece captured_piece = (Piece)piece_at[captured_pos];

    // Little aside before we modify anything, record the destroyable data in the undo stack

    Undo undo = {
        .captured_piece = (uint8_t)captured_piece,
        .flags = {
            sides[0].flags,
            sides[1].flags
        },
        .en_passant_sq = en_passant_sq
    };

    assert(undo_count < MAX_DEPTH);
    undo_stack[undo_count++] = undo;

    if (captured_piece != PIECE_NONE) {
        remove_piece(*this, opponent(to_move), captured_piece, captured_pos);
    }

    // Then move the piece
    // The order matters here

    Piece start_piece = (Piece)piece_at[move_from(move)];
    Piece end_piece = start_piece;

    switch (move_type(move)) {
        case MOVE_PROMOTE_BISHOP:
            end_piece = PIECE_BISHOP;
            break;
        case MOVE_PROMOTE_KNIGHT:
            end_piece = PIECE_KNIGHT;
            break;
        case MOVE_PROMOTE_QUEEN:
            end_piece = PIECE_QUEEN;
            break;
        case MOVE_PROMOTE_ROOK:
            end_piece = PIECE_ROOK;
            break;
    }

    remove_piece(*this, to_move, start_piece, move_from(move));
    set_piece   (*this, to_move, end_piece,   move_to(move));

    // Update castling rights
    
    switch (piece_at[move_from(move)]) {
        case PIECE_KING:
            sides[to_move].set_can_castle_kingside(false);
            sides[to_move].set_can_castle_queenside(false);
            break;

        case PIECE_ROOK:
            uint8_t kingside_rook_pos = to_move == WHITE ? 7 : 63;
            uint8_t queenside_rook_pos = to_move == WHITE ? 0 : 56;

            if (kingside_rook_pos == move_from(move)) {
                sides[to_move].set_can_castle_kingside(false);
            } else if (queenside_rook_pos == move_from(move)) {
                sides[to_move].set_can_castle_queenside(false);
            }

            break;
    }

    en_passant_sq = NULL_SQUARE;

    switch (move_type(move)) {
        case MOVE_DOUBLE_PUSH: {
            int offsets[] = { -8, 8 }; // set en passant marker
            en_passant_sq = move_to(move) + offsets[to_move];
        } break;

        case MOVE_SHORT_CASTLE: {
            auto [p1, p2] = short_castle_start_and_end_rook_squares(to_move);
            remove_piece(*this, to_move, PIECE_ROOK, p1);
            set_piece   (*this, to_move, PIECE_ROOK, p2);
        } break;

        case MOVE_LONG_CASTLE: {
            auto [p1, p2] = long_castle_start_and_end_rook_squares(to_move);
            remove_piece(*this, to_move, PIECE_ROOK, p1);
            set_piece   (*this, to_move, PIECE_ROOK, p2);
        } break;
    }

    to_move = opponent(to_move);
}

void Position::unmake_move(const Move& move) {
    assert(undo_count > 0);
    Undo undo = undo_stack[--undo_count];

    to_move = opponent(to_move);

    Piece end_piece = (Piece)piece_at[move_to(move)];
    Piece start_piece = end_piece;

    switch (move_type(move)) {
        case MOVE_PROMOTE_BISHOP:
        case MOVE_PROMOTE_KNIGHT:
        case MOVE_PROMOTE_QUEEN:
        case MOVE_PROMOTE_ROOK:
            start_piece = PIECE_PAWN;
            break;

        case MOVE_SHORT_CASTLE: {
            auto [p1, p2] = short_castle_start_and_end_rook_squares(to_move);
            remove_piece(*this, to_move, PIECE_ROOK, p2);
            set_piece   (*this, to_move, PIECE_ROOK, p1);
        } break;

        case MOVE_LONG_CASTLE: {
            auto [p1, p2] = long_castle_start_and_end_rook_squares(to_move);
            remove_piece(*this, to_move, PIECE_ROOK, p2);
            set_piece   (*this, to_move, PIECE_ROOK, p1);
        } break;
    }

    remove_piece(*this, to_move, end_piece,   move_to(move));
    set_piece   (*this, to_move, start_piece, move_from(move));

    // replace the captured piece must be done AFTER we remove the moving piece
    if (undo.captured_piece != PIECE_NONE) {
        int captured_square = get_captured_square(move, to_move);
        set_piece(*this, opponent(to_move), (Piece)undo.captured_piece, captured_square);
    }

    sides[0].flags = undo.flags[0];
    sides[1].flags = undo.flags[1];

    en_passant_sq = undo.en_passant_sq;
}