#include <array>

#include "blunderfish.h"

constexpr int64_t MATE_SCORE = 0xffffffff;

int64_t Position::negamax(int depth, int ply) {
    if (depth == 0) {
        return eval();
    }

    int64_t best = -MATE_SCORE;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);
    filter_moves(moves);

    if (moves.empty()) {
        if (is_in_check(to_move)) {
            return -MATE_SCORE + ply; // checkmate
        }
        else {
            return 0; // stalemate
        }
    }

    for (auto& m : moves) {
        make_move(m);
        int64_t score = -negamax(depth - 1, ply + 1);
        unmake_move(m);

        best = std::max(best, score);
    }

    return best;
}

std::optional<Move> Position::best_move() {
    int64_t best_score = INT64_MIN;
    int best_move = -1;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);
    filter_moves(moves);

    for (int i = 0; i < moves.size(); ++i) {
        auto& m = moves[i];

        make_move(m);
        int64_t score = -negamax(2, 1);
        unmake_move(m);

        if (score > best_score) {
            best_score = score;
            best_move = i;
        }
    }

    if (best_move == -1) {
        return std::nullopt;
    }

    return moves[best_move];
}