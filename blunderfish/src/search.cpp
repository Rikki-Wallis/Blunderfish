#include <array>

#include "blunderfish.h"

constexpr int64_t MATE_SCORE = 0xffffff;

int64_t Position::pruned_negamax(int depth, int ply, int64_t alpha, int64_t beta) {
    if (depth == 0) {
        return quiescence(ply, alpha, beta);
    }

    int my_side = to_move;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);

    bool legal_found = false;

    for (Move m : moves) {
        bool cutoff = false;

        make_move(m);

        if (!is_in_check(my_side)) {
            legal_found = true;
            int64_t score = -pruned_negamax(depth - 1, ply + 1, -beta, -alpha);
            alpha = std::max(score, alpha);

            if (alpha >= beta) { // opponent will never allow this; cutoff
                cutoff = true;
            }
        }

        unmake_move(m);

        if (cutoff) {
            return beta; // mate detection not relevant on this node because we found legal moves
        } 
    }

    if (!legal_found) { // no legal moves
        if (is_in_check(my_side)) {
            return -MATE_SCORE + ply; // checkmate
        }
        else {
            return 0; // stalemate
        }
    }

    return alpha;
}

int64_t Position::quiescence(int ply, int64_t alpha, int64_t beta) {
    int side = to_move;
    bool currently_checked = is_in_check(side);

    if (!currently_checked) { // if not checked, we can choose to stay here
        int64_t stand_pat = eval();

        alpha = std::max(stand_pat, alpha);

        if (alpha >= beta) {
            return beta;
        }
    }

    std::array<Move, 256> move_buf;
    std::span<Move> moves = currently_checked ? generate_moves(move_buf) : generate_captures(move_buf);

    bool legal_found = false;

    for (Move mv : moves) {
        make_move(mv);

        bool cutoff = false;

        if (!is_in_check(side)) {
            legal_found = true;

            int64_t score = -quiescence(ply+1, -beta, -alpha);

            alpha = std::max(score, alpha);

            if (alpha >= beta) {
                cutoff = true;
            }
        }

        unmake_move(mv);

        if (cutoff) {
            return beta;
        }
    }

    if (currently_checked && !legal_found) {
        return -MATE_SCORE + ply; // checkmate
    }

    return alpha;
}

int64_t Position::negamax(int depth, int ply) {
    if (depth == 0) {
        return quiescence(ply, INT32_MIN, INT32_MAX);
    }

    int my_side = to_move;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);

    bool legal_found = false;

    int64_t alpha = -MATE_SCORE;

    for (Move m : moves) {
        make_move(m);

        if (!is_in_check(my_side)) {
            legal_found = true;
            int64_t score = -negamax(depth - 1, ply + 1);
            alpha = std::max(score, alpha);
        }

        unmake_move(m);
    }

    if (!legal_found) {
        if (is_in_check(my_side)) {
            return -MATE_SCORE + ply; // checkmate
        }
        else {
            return 0; // stalemate
        }
    }

    return alpha;
}

int Position::best_move(std::span<Move> moves, uint8_t depth) {
    int64_t best_score = INT64_MIN;
    int best_move = -1;

    int64_t alpha = INT32_MIN;
    int64_t beta = INT32_MAX;

    for (int i = 0; i < (int)moves.size(); ++i) {
        Move m = moves[i];

        make_move(m); // no need to filter for check here - assumes filtered moves given
        int64_t score = -pruned_negamax(depth-1, 1, -beta, -alpha);
        unmake_move(m);

        if (score > best_score) {
            best_score = score;
            best_move = i;
        }

        alpha = std::max(alpha, score);
    }

    if (best_move == -1) {
        return -1;
    }

    return best_move;
}