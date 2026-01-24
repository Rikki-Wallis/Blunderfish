#include <cassert>
#include <cctype>

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

static size_t index_from_rank_and_file(size_t rank, size_t file) {
    assert(rank < 8 && file < 8);
    return rank * 8 + file;
}

std::optional<Position> Position::decode_fen_string(const std::string& fen) {
    size_t cur = 0;

    auto set_piece = [](uint64_t* bb, size_t rank, size_t file) {
        size_t index = index_from_rank_and_file(rank, file);
        *bb |= 1 << index;
    };

    auto skip_whitespace = [&]() {
        while (cur < fen.size() && isspace(fen[cur])) {
            cur++;
        }
    };

    auto next = [&]() {
        return fen[cur++];
    };

    Position pos = {};

    skip_whitespace();
    
    for (size_t rank = 7; rank >= 0; --rank) {
        if (rank < 7) {
            if (cur >= fen.size()) return std::nullopt;
            if (next() != '/') return std::nullopt;
        }

        size_t file = 0;

        while (file < 8) {
            if (cur >= fen.size()) return std::nullopt;
            char c = next();

            int upper = isupper(c) != 0;

            switch (c) {
                default:
                    return std::nullopt;

                case 'p':
                case 'P':
                    set_piece(&pos.sides[upper].pawns, rank, file);
                    file++;
                    break;
                case 'r':
                case 'R':
                    set_piece(&pos.sides[upper].rooks, rank, file);
                    file++;
                    break;
                case 'n':
                case 'N':
                    set_piece(&pos.sides[upper].knights, rank, file);
                    file++;
                    break;
                case 'b':
                case 'B':
                    set_piece(&pos.sides[upper].bishops, rank, file);
                    file++;
                    break;
                case 'q':
                case 'Q':
                    set_piece(&pos.sides[upper].queens, rank, file);
                    file++;
                    break;
                case 'k':
                case 'K':
                    set_piece(&pos.sides[upper].king, rank, file);
                    file++;
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                    file += c - '0';
                    break;
            }
        }
    }

    skip_whitespace();
    if (cur >= fen.size()) return std::nullopt;

    switch (next()) {
        default:
            return std::nullopt;
        case 'w': 
            pos.to_move = WHITE;
            break;
        case 'b': 
            pos.to_move = BLACK;
            break;
    }

    return pos;
}