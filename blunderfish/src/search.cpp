#include <array>

#include "blunderfish.h"

constexpr int64_t FUTILITY_MARGIN = 200;

constexpr int64_t MATE_SCORE = 0xffffff;

constexpr int32_t BEST_MOVE_SCORE   = 2000000;
constexpr int32_t CAPTURE_SCORE     = 90000;
constexpr int32_t KILLER_1_SCORE    = 80000;
constexpr int32_t KILLER_2_SCORE    = 70000;
constexpr int32_t MAX_HISTORY_SCORE = 60000;

constexpr uint64_t TT_MASK = TRANSPOSITION_TABLE_SIZE - 1;

enum TTScoreFlag {
    TT_SCORE_EXACT,
    TT_SCORE_UPPER,
    TT_SCORE_LOWER
};

static Move select_best(std::span<Move>& moves, std::span<int32_t>& move_scores, int index) {
    int32_t best_score = INT32_MIN;
    int best_index = -1;

    for (int i = index; i < (int)moves.size(); ++i) {
        int32_t score = move_scores[i];

        if (score > best_score) {
            best_score = score;
            best_index = i;
        }
    }

    assert(best_index != -1);

    std::swap(moves[index], moves[best_index]);
    std::swap(move_scores[index], move_scores[best_index]);

    return moves[index];
}

int32_t Position::mvv_lva_score(Move mv) const{
    int captured_sq = get_captured_square(mv);
    Piece captured_piece = (Piece)piece_at[captured_sq];
    Piece moving_piece = (Piece)piece_at[move_from(mv)];
    int32_t score = CAPTURE_SCORE + piece_value_centipawns(captured_piece) * 1000 - piece_value_centipawns(moving_piece);
    return captured_piece == PIECE_NONE ? 0 : score;
}

bool update_tt_entry(TTEntry& entry, uint64_t zobrist, int depth, int64_t score, int ply, int64_t alpha_original, int64_t beta_original, Move best_move) {
    if (entry.key != zobrist || depth >= entry.depth) {
        entry.key = zobrist;
        entry.depth = depth;
        entry.raw_score = score;

        if (entry.raw_score < -MATE_SCORE + 1000) {
            entry.raw_score -= ply;
        }
        else if (entry.raw_score > MATE_SCORE - 1000) {
            entry.raw_score += ply; // remove the current ply so that the mate score is relative to here rather than the root
        }

        if (score <= alpha_original) {
            entry.flag = TT_SCORE_UPPER; // 
        }
        else if (score >= beta_original) {
            entry.flag = TT_SCORE_LOWER; // true score is AT LEAST best_score
        }
        else {
            entry.flag = TT_SCORE_EXACT; // best_score IS the true score
        }

        entry.best_move = best_move;

        return true;
    }

    return false;
}

static std::span<int32_t> compute_move_scores(Position* pos, HistoryTable& history, KillerTable& killers, int ply, std::span<int32_t> score_buf, std::span<Move> moves, Move best_move) {
    std::span<int32_t> move_scores = score_buf.subspan(0, moves.size());

    for (size_t i = 0; i < moves.size(); ++i) {
        Move mv = moves[i];

        if (mv == best_move) {
            move_scores[i] = BEST_MOVE_SCORE;
        }
        else if (mv == killers[ply][0]) {
            move_scores[i] = KILLER_1_SCORE;
        }
        else if (mv == killers[ply][1]) {
            move_scores[i] = KILLER_2_SCORE;
        }
        else {
            int32_t score = pos->mvv_lva_score(mv);
            Piece piece = (Piece)pos->piece_at[move_from(mv)];
            int to = move_to(mv);
            move_scores[i] = score == 0 ? history[piece][to] : score;
        }
    }

    return move_scores;
}

int64_t Position::pruned_negamax(int depth, TranspositionTable& tt, HistoryTable& history, KillerTable& killers, int ply, bool allow_null, int64_t alpha, int64_t beta) {
    if (depth == 0) {
        return quiescence(ply, alpha, beta);
    }

    int my_side = to_move;

    // First check TT

    TTEntry& entry = tt[zobrist & TT_MASK];

    int64_t alpha_original = alpha; // store this for when we update the transposition table
    int64_t beta_original  = beta; // store this for when we update the transposition table

    Move tt_move = NULL_MOVE;

    if (entry.key == zobrist) {
        if (entry.depth >= depth) {
            int64_t entry_score = entry.raw_score;

            if (entry_score < -MATE_SCORE + 1000) {
                entry_score += ply;
            }
            else if (entry_score >  MATE_SCORE - 1000) {
                entry_score -= ply; // reapply the ply to mate scores to restore them to relative to the root
            }

            switch (entry.flag) {
                case TT_SCORE_EXACT:
                    return entry_score; // exact store stored, we can just return it

                case TT_SCORE_LOWER:
                    alpha = std::max(alpha, entry_score);
                    break;

                case TT_SCORE_UPPER:
                    beta = std::min(beta, entry_score);
                    break;
            }

            if (alpha >= beta) {
                return entry_score;
            }
        }

        tt_move = entry.best_move;
    }

    // Null move pruning
    // if we give the opponent a free move and alpha >= beta, our position is too good, so prune

    int64_t best_score = -MATE_SCORE;

    bool currently_checked = is_in_check(my_side);

    bool skip_null = !allow_null || currently_checked || (total_non_pawn_value() == 0);

    if (depth >= 3 && !skip_null) { 
        make_null_move(); 

        int R = 2; // we subtract this from depth to reduce the search depth

        int64_t null_alpha = beta - 1; // we only care if it can beat it, not the actual score
        int64_t null_beta = beta;
        
        int64_t score = -pruned_negamax(depth - 1 - R, tt, history, killers, ply + 1, false, -null_beta, -null_alpha);

        unmake_null_move();

        if (score >= beta) {
            bool changed = update_tt_entry(entry, zobrist, depth, score, ply, alpha_original, beta_original, NULL_MOVE);
            if (changed) {
                entry.flag = TT_SCORE_LOWER;
            }
            return score;
        }
    }

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);

    std::array<int32_t, 256> score_buf;
    std::span<int32_t> move_scores = compute_move_scores(this, history, killers, ply, score_buf, moves, tt_move);

    bool futility_prune = depth == 1 && !currently_checked && ((eval() + FUTILITY_MARGIN) < alpha); // if quiet moves couldn't possibly improve the position at the leaves by enough, don't search them

    bool legal_found = false;

    Move best_move = NULL_MOVE;

    for (int i = 0; i < (int)moves.size(); ++i) {
        Move m = select_best(moves, move_scores, i);

        bool cutoff = false;

        bool quiet = !is_capture(m);
        bool late = i >= 3;

        int reduction = int(depth >= 3 && quiet && late && !currently_checked);

        if (futility_prune && quiet) {
            continue;
        }

        make_move(m);

        if (!is_in_check(my_side)) {
            legal_found = true;

            int64_t score;

            if (reduction) {
                score = -pruned_negamax(depth - 1 - reduction, tt, history, killers, ply + 1, true, -beta, -alpha); // search at a reduced depth

                if (score > alpha) { // if interesting, full depth search
                    score = -pruned_negamax(depth - 1, tt, history, killers, ply + 1, true, -beta, -alpha);
                }

                // this preserves alpha-beta correctness because we only allow obviously bad moves to be searched shallow
                // if its good, we re-search at full depth
            }
            else {
                score = -pruned_negamax(depth - 1, tt, history, killers, ply + 1, true, -beta, -alpha);
            }

            if (score > best_score) {
                best_score = score;
                best_move = m;
            }

            if (score > alpha) {
                alpha = score;
            }

            if (alpha >= beta) { // opponent will never allow this; cutoff
                cutoff = true;
            }
        }

        unmake_move(m);

        if (cutoff) {
            if (quiet) {
                if (m != killers[ply][0]) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = m;
                }

                Piece piece = (Piece)piece_at[move_from(m)];
                int to = move_to(m);

                history[piece][to] += depth * depth;

                if (history[piece][to] > MAX_HISTORY_SCORE) {
                    history[piece][to] = MAX_HISTORY_SCORE;
                }
            }

            break;
        } 
    }

    if (!legal_found) { // no legal moves
        if (is_in_check(my_side)) {
            best_score = -MATE_SCORE + ply; // checkmate
        }
        else {
            best_score = 0; // stalemate
        }
    }

    update_tt_entry(entry, zobrist, depth, best_score, ply, alpha_original, beta_original, best_move);

    return best_score;
}

int64_t Position::quiescence(int ply, int64_t alpha, int64_t beta) {
    int side = to_move;
    bool currently_checked = is_in_check(side);

    int64_t best_score = -MATE_SCORE;

    if (!currently_checked) { // if not checked, we can choose to stay here
        int64_t stand_pat = eval();
        best_score = stand_pat;

        alpha = std::max(stand_pat, alpha);

        if (alpha >= beta) {
            return stand_pat;
        }
    }

    std::array<Move, 256> move_buf;
    std::span<Move> moves = currently_checked ? generate_moves(move_buf) : generate_captures(move_buf);

    std::array<int32_t, 256> score_buf;
    std::span<int32_t> move_scores = std::span(score_buf).subspan(0, moves.size());

    for (size_t i = 0; i < moves.size(); ++i) {
        Move mv = moves[i];
        move_scores[i] = mvv_lva_score(mv);
    }

    bool legal_found = false;

    for (int i = 0; i < (int)moves.size(); ++i) {
        Move mv = select_best(moves, move_scores, i);

        bool cutoff = false;

        make_move(mv);

        if (!is_in_check(side)) {
            legal_found = true;

            int64_t score = -quiescence(ply+1, -beta, -alpha);

            if (score > best_score) {
                best_score = score;
            }

            if (score > alpha) {
                alpha = score;
            }

            if (alpha >= beta) {
                cutoff = true;
            }
        }

        unmake_move(mv);

        if (cutoff) {
            return best_score;
        }
    }

    if (currently_checked && !legal_found) {
        return -MATE_SCORE + ply; // checkmate
    }

    return best_score;
}

int64_t Position::negamax(int depth, int ply) {
    if (depth == 0) {
        return quiescence(ply, INT32_MIN, INT32_MAX);
    }

    int my_side = to_move;

    std::array<Move, 256> move_buf;
    std::span<Move> moves = generate_moves(move_buf);

    bool legal_found = false;

    int64_t best_score = -MATE_SCORE;

    for (Move m : moves) {
        make_move(m);

        if (!is_in_check(my_side)) {
            legal_found = true;
            int64_t score = -negamax(depth - 1, ply + 1);
            best_score = std::max(score, best_score);
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

    return best_score;
}

Move Position::best_move_internal(std::span<Move> moves, int depth, TranspositionTable& tt, Move last_best_move, HistoryTable& history, KillerTable& killers) {
    (void)last_best_move;

    int ply = 1;

    int64_t best_score = INT64_MIN;
    int best_move = NULL_MOVE;

    int64_t alpha = INT32_MIN;
    int64_t beta = INT32_MAX;

    std::array<int32_t, 256> score_buf;
    std::span<int32_t> move_scores = compute_move_scores(this, history, killers, ply, score_buf, moves, last_best_move);

    for (int i = 0; i < (int)moves.size(); ++i) {
        Move m = select_best(moves, move_scores, i);

        make_move(m); // no need to filter for check here - assumes filtered moves given
        int64_t score = -pruned_negamax(depth-1, tt, history, killers, ply+1, true, -beta, -alpha);
        unmake_move(m);

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }

        alpha = std::max(alpha, score);
    }

    return best_move;
}

Move Position::best_move(std::span<Move> _moves, int depth) {
    KillerTable killers{};
    HistoryTable history{};
    TranspositionTable tt(TRANSPOSITION_TABLE_SIZE);

    Move best_move = NULL_MOVE;

    for (int i = 1; i <= depth; ++i) {
        std::vector<Move> moves(_moves.begin(), _moves.end()); 
        best_move = best_move_internal(moves, i, tt, best_move, history, killers);
    }

    return best_move;
}