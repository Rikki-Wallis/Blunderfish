#include <bit>

#include "blunderfish.h"
#include "../generated/generated_tables.h"

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
    return ((all_pieces & mask) * magic) >> shift;
}

uint64_t rook_moves(uint8_t from, uint64_t all_pieces, uint64_t allies) {
    size_t index = magic_index(all_pieces, rook_mask[from], rook_magic[from], rook_shift[from]);
    uint64_t moves = rook_move[from][index];
    return moves & (~allies);
}

uint64_t bishop_moves(uint8_t from, uint64_t all_pieces, uint64_t allies) {
    size_t index = magic_index(all_pieces, bishop_mask[from], bishop_magic[from], bishop_shift[from]);
    uint64_t moves = bishop_move[from][index];
    return moves & (~allies);
}

uint64_t queen_moves(uint8_t from, uint64_t all_pieces, uint64_t allies) {
    return bishop_moves(from, all_pieces, allies) | rook_moves(from, all_pieces, allies);
}


struct set_bits {
    uint64_t x;

    set_bits(uint64_t x)
        : x(x)
    {}

    struct Iterator {
        uint64_t v;

        bool operator!=(std::default_sentinel_t) const {
            return v != 0;
        }

        Iterator& operator++() {
            v &= v - 1;
            return *this;
        }

        uint8_t operator*() const {
            return (uint8_t)std::countr_zero(v);
        }
    };

    Iterator begin() const { return { .v = x }; }
    std::default_sentinel_t end() const { return {}; }
};

uint64_t Position::generate_attacks(uint8_t colour) const {
    uint64_t allies = sides[colour].all();
    uint64_t attacks = 0;

    for (uint8_t from : set_bits(sides[colour].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, allies))) {
            attacks = attacks | to;
        }
    }

    for (uint8_t from: set_bits(sides[colour].bb[PIECE_ROOK])) {
        for (uint8_t to : set_bits(rook_moves(from, all, allies))) {
            attacks = attacks | to;
        }
    }

    for (uint8_t from: set_bits(sides[colour].bb[PIECE_BISHOP])) {
        for (uint8_t to : set_bits(bishop_moves(from, all, allies))) {
            attacks = attacks | to;
        }
    }

    for (uint8_t from: set_bits(sides[colour].bb[PIECE_QUEEN])) {
        for (uint8_t to : set_bits(queen_moves(from, all, allies))) {
            attacks = attacks | to;
        }
    }

    auto pawn_attacks = colour == WHITE ? white_pawn_attacks : black_pawn_attacks;
    attacks = attacks | pawn_attacks(sides[colour].bb[PIECE_PAWN]);

    return attacks;
}

std::span<Move> Position::generate_moves(std::span<Move> move_buf) const {
    size_t move_count = 0;

    auto new_move = [&](uint8_t from, uint8_t to, uint8_t piece, uint8_t flags) {
        assert("move buffer overflow" && move_count < move_buf.size());

        move_buf[move_count++] = Move {
            .from = from,
            .to = to,
            .piece = piece,
            .flags = flags
        };
    };

    int opp = opponent(to_move);
    uint64_t opps = sides[opp].all();
    uint64_t allies = sides[to_move].all();
    uint64_t all = all_pieces();

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, allies))) {
            new_move(from, to, PIECE_KING, 0);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_ROOK])) {
        for (uint8_t to : set_bits(rook_moves(from, all, allies))) {
            new_move(from, to, PIECE_ROOK, 0);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_BISHOP])) {
        for (uint8_t to : set_bits(bishop_moves(from, all, allies))) {
            new_move(from, to, PIECE_BISHOP, 0);
        }
    }

    for (uint8_t from: set_bits(sides[to_move].bb[PIECE_QUEEN])) {
        for (uint8_t to : set_bits(queen_moves(from, all, allies))) {
            new_move(from, to, PIECE_QUEEN, 0);
        }
    }

    auto pawn_single_moves    = to_move == WHITE ? white_pawn_single_moves : black_pawn_single_moves;
    auto pawn_double_moves    = to_move == WHITE ? white_pawn_double_moves : black_pawn_double_moves;
    int promotion_rank        = to_move == WHITE ? RANK_8 : RANK_1;

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_PAWN])) {
        for (uint8_t to : set_bits(pawn_single_moves(from, opps, all))) {
            
            if (to & promotion_rank) {
                new_move(from, to, PIECE_PAWN, FLAG_PROMOTION);
            } 
            else {
                new_move(from, to, PIECE_PAWN, 0);
            }
        }

        for (uint8_t to : set_bits(pawn_double_moves(from, all))) {
            new_move(from, to, PIECE_PAWN, MOVE_FLAG_DOUBLE_PUSH);
        }

        for (uint8_t to : set_bits(pawn_enpassant_moves(from, en_passant_sq))) {
            new_move(from, to, PIECE_PAWN, MOVE_FLAG_ENPASSANT);
        }
    }

    return move_buf.subspan(0, move_count);
}

std::unordered_map<std::string, size_t> Position::name_moves(std::span<Move> all) const {
    std::vector<size_t> board[64];

    for (size_t i = 0; i < all.size(); ++i) {
        board[all[i].to].push_back(i);
    }

    std::unordered_map<std::string, size_t> lookup;

    for (auto& moves : board) {
        for (size_t i = 0; i < moves.size(); ++i) {
            Move& m = all[moves[i]];

            auto [f1, r1] = square_alg(m.from);

            bool is_capture = (piece_at[m.to] != PIECE_NONE) || (m.flags & MOVE_FLAG_ENPASSANT);

            bool need_file = m.piece == PIECE_PAWN && is_capture;
            bool need_rank = false;

            for (size_t j = 0; j < moves.size(); ++j) {
                if (i == j) {
                    continue;
                }

                Move& n = all[moves[j]];

                if (m.piece != n.piece) {
                    continue;
                }

                auto [f2, r2] = square_alg(n.from);

                if (f1 != f2) {
                    need_file = true;
                }
                else if (r1 != r2) {
                    need_rank = true;
                }
            }

            auto [to_file, to_rank] = square_alg(m.to);

            std::string capture_str = is_capture ? "x" : "";
            std::string unambig_file = need_file ? std::format("{}", f1) : "";
            std::string unambig_rank = need_rank ? std::format("{}", r1) : "";

            std::string name = std::format("{}{}{}{}{}{}", piece_alg_table[m.piece], unambig_file, unambig_rank, capture_str, to_file, to_rank);

            lookup.emplace(std::move(name), moves[i]);
        } 
    }

    return lookup;
}

Position Position::execute_move(const Move& move) const {
    Position next = {};
    memcpy(next.sides, sides, sizeof(sides));
    memcpy(next.piece_at, piece_at, sizeof(piece_at));
    next.to_move = opponent(to_move);

    next.sides[to_move].bb[move.piece] &= ~sq_to_bb(move.from);
    next.sides[to_move].bb[move.piece] |= sq_to_bb(move.to);
    
    int captured_pos;

    if (move.flags & MOVE_FLAG_ENPASSANT) {
        int offsets[] = { -8, 8 };
        captured_pos = move.to + offsets[to_move];
    }
    else {
        captured_pos = move.to; 
    }

    Piece captured_piece = (Piece)next.piece_at[captured_pos];

    if (captured_piece != PIECE_NONE) {
        next.sides[opponent(to_move)].bb[captured_piece] &= ~sq_to_bb(captured_pos);
        next.piece_at[captured_pos] = PIECE_NONE;
    }

    next.piece_at[move.from] = PIECE_NONE;
    next.piece_at[move.to] = move.piece; // make sure this happens AFTER we set the state of the captured piece because they could be on the same square
    
    if (move.piece == PIECE_KING) {
        next.sides[to_move].set_can_castle_kingside(false);
        next.sides[to_move].set_can_castle_queenside(false);
    }

    if (move.flags & MOVE_FLAG_DOUBLE_PUSH ) {
        int offsets[] = { -8, 8 };
        next.en_passant_sq = move.to + offsets[to_move];
    }

    if (move.flags & FLAG_PROMOTION) {
        // TODO: this
    }

    return next;
}