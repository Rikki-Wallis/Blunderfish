#include <random>

#include "blunderfish.h"

constexpr size_t COUNT = 1024*128;

constexpr size_t TSET_SIZE = size_t(1) << 20;
constexpr size_t TSET_MASK = TSET_SIZE - 1;

constexpr int opening_depth = 16;
constexpr int eval_depth = 14;

using TSet = Bitset<TSET_SIZE>;


static std::mt19937 engine{std::random_device{}()};

static void explore(FILE* stream, Position& pos, int depth, TSet& seen, size_t& count) {

    if (depth == 0) {
        if (seen.get(pos.zobrist & TSET_MASK)) {
            return;
        }
        std::string fen = pos.fen();

        fprintf(stream, "%s\n", fen.c_str());

        count++;
        seen.set(pos.zobrist & TSET_MASK);
    } 
    else {
        std::array<Move, 256> move_buf;
        std::span<Move> moves = pos.generate_moves(move_buf);
        pos.filter_moves(moves);

        if (moves.size() != 0) {
            std::uniform_int_distribution<size_t> dist{0, moves.size()-1};
            size_t idx = dist(engine);
            Move mv = moves[idx];
            pos.make_move(mv);
            explore(stream, pos, depth-1, seen, count);
            pos.unmake_move();
        }
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
        explore(file, pos, opening_depth, seen, count);
    }

    fclose(file);

    print("Generated {} positions.\n", COUNT);

    return 0;
}