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

std::span<Move> Position::generate_moves(int side, std::span<Move> move_buf) const {
    assert("invalid side" && side < 2);

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

    for (uint8_t from : set_bits(sides[side].king)) {
        for (uint8_t to : set_bits(king_moves(from, sides[side].all()))) {
            new_move(from, to, PIECE_KING, 0);
        }
    }

    return move_buf.subspan(0, move_count);
}