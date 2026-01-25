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

int main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 

    auto maybe_pos = Position::decode_fen_string(start_fen);
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
        return 1;
    }

    Position pos = *maybe_pos;

    for (;;) {
        pos.display();

        std::array<Move, 256> move_buffer;
        std::span<Move> moves = pos.generate_moves(move_buffer);
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

        auto& m = moves[select_move(move_names)];

        pos = pos.execute_move(m);
    }

    //for (size_t i = 0; i < 64; ++i) {
    //    Position pos = {};
    //    pos.sides[WHITE].bb[PIECE_PAWN] = rook_masks[i];
    //    pos.display();
    //}

    return 0;
}