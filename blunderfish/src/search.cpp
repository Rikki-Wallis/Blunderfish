#include <array>

#include "blunderfish.h"

constexpr int64_t FUTILITY_MARGIN = 200;

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
    if (entry.key != zobrist || depth > entry.depth) {
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

static TTEntry& find_entry(TranspositionTable& tt, uint64_t zobrist) {
    TTCluster& cluster = tt[zobrist & TT_MASK];

    // find match
    for (auto& e : cluster.entries) {
        if (e.key == zobrist) {
            return e;
        }
    }

    // find empty
    for (auto& e : cluster.entries) {
        if (e.key == 0) {
            return e;
        }
    }

    size_t shallowest = 0;

    for (size_t i = 1; i < std::size(cluster.entries); ++i) {
        if (cluster.entries[i].depth < cluster.entries[shallowest].depth) {
            shallowest = i;
        }
    }
    
    return cluster.entries[shallowest];
}

int64_t Position::pruned_negamax(int depth, TranspositionTable& tt, HistoryTable& history, KillerTable& killers, int ply, bool allow_null, int64_t alpha, int64_t beta) {
    (void)allow_null;

    if (depth == 0) {
        //return eval();
        return quiescence(ply, alpha, beta);
    }

    int my_side = to_move;

    // First check TT

    TTEntry& match = find_entry(tt, zobrist);

    int64_t alpha_original = alpha; // store this for when we update the transposition table
    int64_t beta_original  = beta; // store this for when we update the transposition table

    Move tt_move = NULL_MOVE;

    if (match.key == zobrist) { // exact match
        if (match.depth >= depth) {
            int64_t entry_score = match.raw_score;

            if (entry_score < -MATE_SCORE + 1000) {
                entry_score += ply;
            }
            else if (entry_score >  MATE_SCORE - 1000) {
                entry_score -= ply; // reapply the ply to mate scores to restore them to relative to the root
            }

            switch (match.flag) {
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

        tt_move = match.best_move;
    }

    int64_t best_score = -MATE_SCORE;
    bool currently_checked = is_in_check(my_side);

    // Null move pruning
    // if we give the opponent a free move and alpha >= beta, our position is too good, so prune

    bool low_material = total_non_pawn_value() <= 2 * piece_value_centipawns(PIECE_KNIGHT);
    bool skip_null = !allow_null || currently_checked || low_material;

    int R = 2; // we subtract this from depth to reduce the search depth

    if (depth > R + 1 && !skip_null) { 
        make_null_move(); 

        int64_t null_alpha = beta - 1; // we only care if it can beat it, not the actual score
        int64_t null_beta = beta;
        
        int64_t score = -pruned_negamax(depth - 1 - R, tt, history, killers, ply + 1, false, -null_beta, -null_alpha);

        unmake_null_move();

        if (score >= beta) {
            TTEntry& target = find_entry(tt, zobrist);
            bool changed = update_tt_entry(target, zobrist, depth, beta, ply, alpha_original, beta_original, NULL_MOVE);
            if (changed) { target.flag = TT_SCORE_LOWER; }
            return beta;
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

        int reduction = 0;

        if (depth >= 3 &&
            quiet &&
            !currently_checked &&
            i >= 3) {

            reduction = 1;

            if (depth >= 6 && i >= 6)
                reduction = 2;
        }

        if (futility_prune && quiet) {
            continue;
        }

        make_move(m);

        if (!is_in_check(my_side)) {
            legal_found = true;

            int64_t score;

            if (reduction) {
                score = -pruned_negamax(depth - 1 - reduction, tt, history, killers, ply + 1, true, -alpha-1, -alpha); // search at a reduced depth

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

    TTEntry& target = find_entry(tt, zobrist);
    update_tt_entry(target, zobrist, depth, best_score, ply, alpha_original, beta_original, best_move);

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
        return quiescence(ply, -INF, INF);
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

std::pair<Move, int64_t> Position::best_move_internal(std::span<Move> moves, int depth, TranspositionTable& tt, Move last_best_move, HistoryTable& history, KillerTable& killers, int64_t alpha, int64_t beta) {
    (void)last_best_move;

    int ply = 1;

    int64_t best_score = -INF;
    int best_move = NULL_MOVE;

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

    return {best_move, best_score};
}

Move Position::best_move(std::span<Move> moves, int depth) {
    KillerTable killers{};
    HistoryTable history{};
    TranspositionTable tt;

    Move best_move = NULL_MOVE;
    int64_t best_score = 0;

    for (int i = 1; i <= depth; ++i) {
        int64_t window = 50; // start the window small

        int64_t alpha = best_score - window;
        int64_t beta  = best_score + window;

        while (true) {
            auto [move, score] = best_move_internal(moves, i, tt, best_move, history, killers, alpha, beta);

            if (score <= alpha) {
                // fail low
                alpha -= window;
                window *= 2;
            }
            else if (score >= beta) {
                // fail high
                beta += window;
                window *= 2;
            }
            else {
                best_move = move;
                best_score = score;
                break;
            }
        }
    }

    return best_move;
}