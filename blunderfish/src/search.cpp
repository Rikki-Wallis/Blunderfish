#include <array>

#include "blunderfish.h"

constexpr int64_t MATE_SCORE = 0xffffffff;

// alpha is, the score that the current player can already guarantee so far
// beta is the score that the opponent can guarantee so far

int64_t Position::negamax(int depth, int ply, int64_t alpha, int64_t beta) {
    if (depth == 0) {
        return eval();
    }

    int my_side = to_move;

    int64_t best = -MATE_SCORE;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);

    int n = 0;

    for (Move m : moves) {
        make_move(m);

        bool need_to_break = false;

        if (!is_in_check(my_side)) {
            n++;
            int64_t score = -negamax(depth - 1, ply + 1, -beta, -alpha);
            best = std::max(best, score);
            alpha = std::max(score, alpha);

            if (alpha >= beta) { // we are getting a score that is worse for our opponent, than a score they can already guarantee, so all other branches are irrelevant.
                need_to_break = true;
            }
        }

        unmake_move(m);

        if (need_to_break) {
            break;
        }
    }

    if (n == 0) { // no legal moves
        if (is_in_check(my_side)) {
            return -MATE_SCORE + ply; // checkmate
        }
        else {
            return 0; // stalemate
        }
    }

    return best;
}

int Position::best_move(std::span<Move> moves, uint8_t depth) {
    int64_t best_score = INT64_MIN;
    int best_move = -1;

    for (int i = 0; i < (int)moves.size(); ++i) {
        Move m = moves[i];

        make_move(m);

        //if (!is_in_check()

        int64_t score = -negamax(depth-1, 1, INT64_MIN, INT64_MAX);
        unmake_move(m);

        if (score > best_score) {
            best_score = score;
            best_move = i;
        }
    }

    if (best_move == -1) {
        return -1;
    }

    return best_move;
}