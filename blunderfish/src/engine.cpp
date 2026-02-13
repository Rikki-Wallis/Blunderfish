#include <cstdio>
#include <array>
#include "blunderfish.h"

uint64_t perft_search(int depth, Position& position) {
    
    if (depth == 0) {
        return 1;
    }

    uint64_t nodes = 0;
    std::array<Move, 256> move_buffer;
    std::span<Move> moves = position.generate_moves(move_buffer);
    position.filter_moves(moves);

    for (const Move& move : moves) {
        position.make_move(move);
        nodes += perft_search(depth-1, position);
        position.unmake_move(move);
    }

    return nodes;
}