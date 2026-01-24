#include <cstdio>

#include "position.h"

int main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 

    auto maybe_pos = Position::decode_fen_string(start_fen);
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
        return 1;
    }

    auto pos = *maybe_pos;
    pos.display();

    return 0;
}