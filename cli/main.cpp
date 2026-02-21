#include <cstdio>
#include <iostream>
#include <array>

#include "blunderfish.h"

static size_t select_move(const std::unordered_map<std::string, size_t>& moves) {
    for (;;) {
        print("Enter a valid move: ");

        std::string input;
        std::cin >> input;

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
        pos.display(false);

        std::array<Move, 256> move_buffer;
        std::span<Move> moves = pos.generate_moves(move_buffer);
        pos.filter_moves(moves);
        std::unordered_map<std::string, size_t> move_names = pos.name_moves(moves); 

        if (moves.size() == 0) {
            print("Game over.\n");
            return 0;
        }

        if (pos.to_move == WHITE) {
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

            Move m = moves[selected];
            pos.make_move(m);
        }
        else {
            int move_idx = pos.best_move(moves, 6);
            assert(move_idx != -1);

            for (auto& [name, m] : move_names) {
                if (m == (size_t)move_idx) {
                    print("Blunderfish plays {}\n", name);
                }
            }

            Move m = moves[move_idx];
            pos.make_move(m);
        }
    }

    return 0;
}

static int eval_main(const char* FEN) {
    auto maybe_pos = Position::decode_fen_string(FEN);

    if (!maybe_pos) {
        print("Invalid FEN string '{}'.\n", FEN);
        return 1;
    }

    Position pos = std::move(*maybe_pos);
    int64_t v = pos.eval();

    print("{}\n", v);

    return 0;
}

static int best_main(const char* FEN) {
    auto maybe_pos = Position::decode_fen_string(FEN);

    if (!maybe_pos) {
        print("Invalid FEN string '{}'.\n", FEN);
        return 1;
    }

    Position pos = std::move(*maybe_pos);

    std::array<Move, 256> move_buf;
    std::span<Move> moves = pos.generate_moves(move_buf);
    pos.filter_moves(moves);
    auto names = pos.name_moves(moves);

    int best = pos.best_move(moves, 6);

    if (best == -1) {
        print("There is no move.\n");
        return 0;
    }

    for (auto& [name, move] : names) {
        if (move == (size_t)best) {
            print("{}\n", name);
            return 0;
        }
    }

    assert(false); // should be unreachable
    return 1;
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