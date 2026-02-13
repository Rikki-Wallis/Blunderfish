#include <cstdio>
#include <iostream>
#include <array>

#include "blunderfish.h"
#include "../generated/generated_tables.h"

constexpr size_t UNDO_MOVE = 0xffffffff; 

std::vector<Move> move_stack;

static size_t select_move(const std::unordered_map<std::string, size_t>& moves) {
    for (;;) {
        print("Enter a valid move: ");

        std::string input;
        std::cin >> input;

        if (input == "undo") {
            if (move_stack.size() > 0) {
                return UNDO_MOVE;
            }
        }

        if (moves.contains(input)) {
            return moves.at(input);
        }
    }
}

static int play_main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 

    auto maybe_pos = Position::decode_fen_string(start_fen);
    if (!maybe_pos) {
        print("Invalid FEN string.\n");
        return 1;
    }

    Position pos(std::move(*maybe_pos));

    for (;;) {
        pos.display(true);

        std::array<Move, 256> move_buffer;
        std::span<Move> moves = pos.generate_moves(move_buffer);
        pos.filter_moves(moves);
        std::unordered_map<std::string, size_t> move_names = pos.name_moves(moves); 

        int count = 0;

        print("Available moves: ");
        for (auto& [name, i] : move_names) {
            if (count++ > 0) {
                print(", ");
            }
            print("{}", name);
        }
        print("\n");

        size_t selected = select_move(move_names); 

        if (selected == UNDO_MOVE) {
            Move last_move = move_stack.back();
            move_stack.pop_back();
            pos.unmake_move(last_move);
        }
        else{
            auto& m = moves[selected];
            pos.make_move(m);
            move_stack.push_back(m);
        }
    }

    /*
    for (uint8_t i = 0; i < 64; ++i) {
        Position pos = {};

        pos.sides[BLACK].bb[PIECE_KNIGHT] = (uint64_t)1 << i;
        pos.piece_at[i] = PIECE_KNIGHT;

        uint64_t moves = knight_moves(i, 0);
        pos.sides[WHITE].bb[PIECE_PAWN] = moves;

        for (auto x : set_bits(moves)) {
            pos.piece_at[x] = PIECE_PAWN;
        }

        pos.display();
    }
    */

    return 0;
}

static int eval_main(const char* FEN) {
    auto maybe_pos = Position::decode_fen_string(FEN);

    if (!maybe_pos) {
        print("Invalid FEN string '{}'.\n", FEN);
        return 1;
    }

    Position pos = std::move(*maybe_pos);
    int v = pos.eval();

    print("Position eval: {}\n", v);

    return 0;
}

static int best_main(const char* FEN) {
    (void)FEN;
    print("Best move: bruh!\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: {} <play, eval, best> <FEN?>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "play") == 0) {
        return play_main();
    }

    if (strcmp(argv[1], "eval") == 0) {
        if (argc < 3) {
            print("Usage: {} eval <FEN>\n", argv[0]);
            return 1;
        }

        return eval_main(argv[2]);
    }

    if (strcmp(argv[1], "best") == 0) {
        if (argc < 3) {
            print("Usage: {} best <FEN>\n", argv[0]);
            return 1;
        }

        return best_main(argv[2]);
    }

    print("Invalid mode given '{}'\n", argv[1]);
    return 1;
}