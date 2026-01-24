#include <cstdio>

#include "position.h"

int main() {
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 

    auto pos = Position::decode_fen_string(start_fen);
    pos.display();

    return 0;
}