#include <cassert>
#include <cctype>

#include "position.h"

void Position::display() const {
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