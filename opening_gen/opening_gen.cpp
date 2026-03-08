#include <random>

#include "blunderfish.h"

constexpr size_t COUNT = 1024;

constexpr size_t TSET_SIZE = size_t(1) << 20;
constexpr size_t TSET_MASK = TSET_SIZE - 1;

constexpr int depth = 16;

constexpr int64_t SCORE_THRESHOLD = 150;

using TSet = Bitset<TSET_SIZE>;


static std::mt19937 engine{std::random_device{}()};

static void explore(FILE* stream, Position& pos, int depth, TSet& seen, size_t& count) {

    if (depth == 0) {
        if (seen.get(pos.zobrist & TSET_MASK)) {
            return;
        }

        TranspositionTable tt{};
        HistoryTable history{};
        KillerTable killers{};
        EvalHistory eval_history{};

        pos.reset_benchmarking_statistics();
        int64_t score = pos.pruned_negamax(10, tt, history, killers, eval_history, 1, true, -INF, INF, NULL_MOVE, 0, 10);

        if (std::abs(score) < SCORE_THRESHOLD) {
            std::string fen = pos.fen();
            fprintf(stream, "%s\n", fen.c_str());
            print("{}\n", fen);
            count++;
            seen.set(pos.zobrist & TSET_MASK);
        }
    } 
    else {
        std::array<Move, 256> move_buf;
        std::span<Move> moves = pos.generate_moves(move_buf);
        pos.filter_moves(moves);

        std::uniform_int_distribution<size_t> dist{0, moves.size()-1};
        size_t idx = dist(engine);
        Move mv = moves[idx];
        pos.make_move(mv);
        explore(stream, pos, depth-1, seen, count);
        pos.unmake_move(mv);
    }
}

int main() {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    TSet seen;

    FILE* file = fopen("openings.txt", "w");
    if (!file) {
        print("Failed to open openings.txt\n");
        return 1;
    }

    size_t count = 0;

    while (count < COUNT) {
        explore(file, pos, depth, seen, count);
    }

    fclose(file);

    return 0;
}