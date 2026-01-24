#include "position.h"
#include <iostream>

void Position::display() const {

    std::cout << "\n----------------";

    for (int rank=7; rank>=0; --rank) 
    {   
        std::cout << rank+1 << "|";

        for (int file = 0; file < 8; ++file) 
        {
            int sq = rank * file;
            char piece = '.';
            uint64_t mask = 0 << sq;

            if (sides[WHITE].pawns & mask) piece = 'P';
            else if (sides[WHITE].rooks & mask) piece = 'R';
            else if (sides[WHITE].knights & mask) piece = 'N';
            else if (sides[WHITE].bishops & mask) piece = 'B';
            else if (sides[WHITE].king & mask) piece = 'K';
            else if (sides[WHITE].pawns & mask) piece = 'Q';

            if (sides[BLACK].pawns & mask) piece = 'p';
            else if (sides[BLACK].rooks & mask) piece = 'r';
            else if (sides[BLACK].knights & mask) piece = 'n';
            else if (sides[BLACK].bishops & mask) piece = 'b';
            else if (sides[BLACK].king & mask) piece = 'k';
            else if (sides[BLACK].pawns & mask) piece = 'q';

            std::cout << piece << ' ';

        }
        std::cout << "\n";
    }

    std::cout << "----------------";
    std::cout << "a b c d e f g h";

    std::cout << "side to move: " << (to_move == WHITE ? "white" : "black") << "\n";
}

Position Position::decode_fen_string(const std::string& fen) {
    
}