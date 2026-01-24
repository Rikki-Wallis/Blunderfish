#include <cstdio>

#include "position.h"

int main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 
    //std::string test_fen = "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2";

    auto maybe_pos = Position::decode_fen_string(start_fen);
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
        return 1;
    }

    auto pos = *maybe_pos;
    pos.display(true);

    return 0;
}