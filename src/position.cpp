#include <cassert>
#include <cctype>

#include "position.h"
#include "common.h"

void Position::display() const {
    for (int rank=7; rank>=0; --rank) 
    {   
        print("{}|", rank+1);

        for (int file = 0; file < 8; ++file) 
        {
            int sq = rank * 8 + file;
            const char* piece = ".";
            uint64_t mask = (uint64_t)1 << sq;

            if (sides[WHITE].pawns & mask) piece = "♟";
            else if (sides[WHITE].rooks & mask) piece = "♜";
            else if (sides[WHITE].knights & mask) piece = "♞";
            else if (sides[WHITE].bishops & mask) piece = "♝";
            else if (sides[WHITE].king & mask) piece = "♚";
            else if (sides[WHITE].queens & mask) piece = "♛";

            if (sides[BLACK].pawns & mask) piece = "♙";
            else if (sides[BLACK].rooks & mask) piece = "♖";
            else if (sides[BLACK].knights & mask) piece = "♘";
            else if (sides[BLACK].bishops & mask) piece = "♗";
            else if (sides[BLACK].king & mask) piece = "♔";
            else if (sides[BLACK].queens & mask) piece = "♕";

            print("{} ", piece);

        }

        print("\n");
    }

    print("  ---------------\n");
    print("  a b c d e f g h\n");

    print("To move: {}\n", to_move == WHITE ? "white" : "black");
}

std::optional<Position> Position::decode_fen_string(const std::string& fen) {
    size_t cur = 0;

    auto set_piece = [](uint64_t* bb, size_t rank, size_t file) {
        size_t index = rank * 8 + file;
        *bb |= (uint64_t)1 << index;
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
    
    for (int rank = 7; rank >= 0; --rank) {
        if (rank < 7) {
            if (cur >= fen.size()) return std::nullopt;
            if (next() != '/') return std::nullopt;
        }

        size_t file = 0;

        while (file < 8) {
            if (cur >= fen.size()) return std::nullopt;
            char c = next();

            int side = isupper(c) == 0;

            switch (c) {
                default:
                    return std::nullopt;

                case 'p':
                case 'P':
                    set_piece(&pos.sides[side].pawns, rank, file);
                    file++;
                    break;
                case 'r':
                case 'R':
                    set_piece(&pos.sides[side].rooks, rank, file);
                    file++;
                    break;
                case 'n':
                case 'N':
                    set_piece(&pos.sides[side].knights, rank, file);
                    file++;
                    break;
                case 'b':
                case 'B':
                    set_piece(&pos.sides[side].bishops, rank, file);
                    file++;
                    break;
                case 'q':
                case 'Q':
                    set_piece(&pos.sides[side].queens, rank, file);
                    file++;
                    break;
                case 'k':
                case 'K':
                    set_piece(&pos.sides[side].king, rank, file);
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