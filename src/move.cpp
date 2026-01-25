#include <bit>

#include "blunderfish.h"

inline uint64_t sq_to_bb(uint8_t index) {
    return (uint64_t)1 << index;
}

uint64_t king_moves(uint8_t from, uint64_t side_pieces) {
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

    return m & (~side_pieces);
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

    for (uint8_t from : set_bits(sides[to_move].king)) {
        for (uint8_t to : set_bits(king_moves(from, sides[to_move].all()))) {
            new_move(from, to, PIECE_KING, 0);
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

            bool need_file = m.piece == PIECE_PAWN;
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

            std::string capture_str = (sides[to_move].all() & sq_to_bb(m.to)) ? "x" : "";
            std::string file_str = need_file ? std::format("{}", f1) : "";
            std::string rank_str = need_rank ? std::format("{}", r1) : "";

            std::string name = std::format("{}{}{}{}{}{}", piece_alg[m.piece], file_str, rank_str, capture_str, to_file, to_rank);

            assert(!lookup.contains(name));
            lookup.emplace(std::move(name), i);
        } 
    }

    return lookup;
}