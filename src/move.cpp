#include <bit>

#include "blunderfish.h"

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

    uint64_t push  = (bb << 8) & (~all_pieces);
    uint64_t left  = (bb << 7) & (~FILE_H) & opps;
    uint64_t right = (bb << 9) & (~FILE_A) & opps;

    return push | left | right;
}

uint64_t black_pawn_single_moves(uint8_t from, uint64_t opps, uint64_t all_pieces) {
    uint64_t bb = sq_to_bb(from);

    uint64_t push  = (bb >> 8) & (~all_pieces);
    uint64_t left  = (bb >> 9) & (~FILE_H) & opps;
    uint64_t right = (bb >> 7) & (~FILE_A) & opps;

    return push | left | right;
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
    uint64_t all = all_pieces();

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_KING])) {
        for (uint8_t to : set_bits(king_moves(from, sides[to_move].all()))) {
            new_move(from, to, PIECE_KING, 0);
        }
    }

    auto pawn_single_moves = to_move == WHITE ? white_pawn_single_moves : black_pawn_single_moves;
    auto pawn_double_moves = to_move == WHITE ? white_pawn_double_moves : black_pawn_double_moves;

    for (uint8_t from : set_bits(sides[to_move].bb[PIECE_PAWN])) {
        for (uint8_t to : set_bits(pawn_single_moves(from, opps, all))) {
            new_move(from, to, PIECE_PAWN, 0);
        }

        for (uint8_t to : set_bits(pawn_double_moves(from, all))) {
            new_move(from, to, PIECE_PAWN, 0);
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

            bool is_capture = piece_at[m.to] != PIECE_NONE;

            //if (m.piece == PIECE_PAWN && r1 == 4 && f1 == 'e') {
            //    asm("int3");
            //}

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
    
    Piece captured = (Piece)next.piece_at[move.to]; 

    if (captured != PIECE_NONE) {
        next.sides[opponent(to_move)].bb[captured] &= ~sq_to_bb(move.to);
    }

    next.piece_at[move.from] = PIECE_NONE;
    next.piece_at[move.to] = move.piece;
    
    if (move.piece == PIECE_KING) {
        next.sides[to_move].set_can_castle_kingside(false);
        next.sides[to_move].set_can_castle_queenside(false);
    }

    return next;
}