#include <cstdio>
#include <iostream>
#include <array>

#include "blunderfish.h"

static Move select_move(const std::unordered_map<std::string, Move>& moves) {
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

        std::unordered_map<std::string, Move> names = pos.name_moves(moves); 

        if (moves.size() == 0) {
            print("Game over.\n");
            return 0;
        }

        if (pos.to_move == WHITE) {
            int count = 0;

            print("Available moves: ");
            for (auto& [name, mv] : names) {
                if (count++ > 0) {
                    print(", ");
                }
                print("{}", name);
            }
            print("\n");

            Move m = select_move(names); 
            pos.make_move(m);
        }
        else {
            Move best = pos.best_move(moves, 14);
            assert(best != NULL_MOVE);

            for (auto& [name, mv] : names) {
                if (mv == best) {
                    print("Blunderfish plays {}\n", name);
                }
            }

            pos.make_move(best);
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
    int64_t v = pos.compute_eval();

    print("{}\n", v);

    return 0;
}

static int perft_main(const char* FEN, int depth) {
    auto maybe_pos = Position::decode_fen_string(FEN);

    if (!maybe_pos) {
        print("Invalid FEN string '{}'.\n", FEN);
        return 1;
    }

    Position pos = std::move(*maybe_pos);

    uint64_t count = perft_search(depth, pos);
    print("{}\n", count);

    return 0;
}

static int best_main(const char* FEN, int depth, std::optional<double> time_limit) {
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

    Move best = pos.best_move(moves, depth, time_limit);

    if (best == NULL_MOVE) {
        print("There is no move.\n");
        return 0;
    }

    print("Depth reached: {}\n", pos.max_ply);

    for (auto& [name, mv] : names) {
        if (mv == best) {
            print("{}\n", name);
            return 0;
        }
    }

    assert(false); // should be unreachable
    return 1;
}

static int moves_main(const std::string& FEN) {
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

    int i = 0;
    for (auto& [name, m] : names) {
        if (i > 0) {
            print(", ");
        }
        print("{}", name);
        i++;
    }
    print("\n");

    print("White checked: {}\n", pos.is_checked[WHITE] ? "true" : "false");
    print("Black checked: {}\n", pos.is_checked[BLACK] ? "true" : "false");
    
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

    if (strcmp(argv[1], "perft") == 0) {
        if (argc < 4) {
            print("Usage: {} perft <FEN> <depth>\n", argv[0]);
            return 1;
        }

        const char* FEN = argv[2];

        int depth = atoi(argv[3]);
        if (depth == 0) {
            print("Invalid depth '{}'\n", argv[3]);
            return 1;
        }

        return perft_main(FEN, depth);
    }

    if (strcmp(argv[1], "best") == 0) {
        if (argc < 3) {
            print("Usage: {} best <FEN>\n", argv[0]);
            return 1;
        }

        int depth = 14;

        if (argc >= 4) {
            depth = atoi(argv[3]);
            if (depth < 6) {
                depth = 6;
            }
        }

        std::optional<double> time_limit = std::nullopt;

        if (argc >= 5) {
            double lim = atof(argv[4]);
            if (lim < 0.001) {
                print("Invalid time limit '{}'\n", argv[4]);
                return 1;
            }
            time_limit = lim;
        }

        return best_main(argv[2], depth, time_limit);
    }

    if (strcmp(argv[1], "moves") == 0) {
        if (argc < 3) {
            print("Usage: {} moves <FEN>\n", argv[0]);
            return 1;
        }

        return moves_main(argv[2]);
    }

    print("Invalid mode given '{}'\n", argv[1]);
    return 1;
}