#include <cstdio>
#include <array>

#include "blunderfish.h"

int main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 
    //std::string test_fen = "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2";

    auto maybe_pos = Position::decode_fen_string(start_fen);
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
        return 1;
    }

    Position pos = *maybe_pos;
    memset(pos.sides, 0, sizeof(pos.sides));
    pos.sides[WHITE].king = 1 << 27;
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

    return 0;
}