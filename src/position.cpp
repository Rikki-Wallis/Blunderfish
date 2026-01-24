#include <cassert>
#include <cctype>

#include "position.h"
#include "common.h"

enum SideFlags {
    SIDE_FLAG_NONE = 0,
    SIDE_FLAG_CAN_CASTLE_KINGSIDE  = (1 << 0),
    SIDE_FLAG_CAN_CASTLE_QUEENSIDE = (1 << 1),
};


void Side::set_can_castle_kingside(bool value) {
    if (value) {
        flags |= SIDE_FLAG_CAN_CASTLE_KINGSIDE;
    }
    else {
        flags &= ~SIDE_FLAG_CAN_CASTLE_KINGSIDE;
    }
}

void Side::set_can_castle_queenside(bool value) {
    if (value) {
        flags |= SIDE_FLAG_CAN_CASTLE_QUEENSIDE;
    }
    else {
        flags &= ~SIDE_FLAG_CAN_CASTLE_QUEENSIDE;
    }
}

bool Side::can_castle_kingside() const {
    return (flags & SIDE_FLAG_CAN_CASTLE_KINGSIDE) != 0;
}

bool Side::can_castle_queenside() const {
    return (flags & SIDE_FLAG_CAN_CASTLE_QUEENSIDE) != 0;
}

void Position::display(bool display_metadata) const {
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

    if (display_metadata) {
        for (int side = 0; side < 2; ++side) {
            const char* name_table[] = {
                "White",
                "Black"
            };

            const char* name = name_table[side];

            if (sides[side].can_castle_kingside()) {
                print("{} can castle kingside.\n", name);
            }

            if (sides[side].can_castle_queenside()) {
                print("{} can castle queenside.\n", name);
            }
        }

        if (en_passant_sq != NULL_SQUARE) {
            int sq = en_passant_sq;
            int rank = sq / 8;
            char file = (sq % 8) + 'a';
            print("En passant possible on {}{}.\n", file, rank);
        }
    }

    print("To move: {}\n", to_move == WHITE ? "white" : "black");
}

std::optional<Position> Position::decode_fen_string(const std::string& fen) {
    size_t cursor = 0;

    auto set_piece = [](uint64_t* bb, size_t rank, size_t file) {
        size_t index = rank * 8 + file;
        *bb |= (uint64_t)1 << index;
    };

    auto skip_whitespace = [&]() {
        while (cursor < fen.size() && isspace(fen[cursor])) {
            cursor++;
        }
    };

    auto next = [&]() {
        return fen[cursor++];
    };
    
    auto peek = [&]() {
        return fen[cursor];
    };

    Position pos = {};

    // Piece placement

    skip_whitespace();
    
    for (int rank = 7; rank >= 0; --rank) {
        if (rank < 7) {
            if (cursor >= fen.size()) return std::nullopt;
            if (next() != '/') return std::nullopt;
        }

        size_t file = 0;

        while (file < 8) {
            if (cursor >= fen.size()) return std::nullopt;
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

    // To move

    skip_whitespace();
    if (cursor >= fen.size()) return std::nullopt;

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

    // Castling ability

    skip_whitespace();
    if (cursor >= fen.size()) return std::nullopt;

    if (peek() != '-') {
        while (cursor < fen.size() && !isspace(peek())) {
            char c = next();

            int side = isupper(c) == 0;

            switch(c) {
                default:
                    return std::nullopt;

                case 'k':
                case 'K':
                    pos.sides[side].set_can_castle_kingside(true);
                    break;
                case 'q':
                case 'Q':
                    pos.sides[side].set_can_castle_queenside(true);
                    break;
            }
        }
    }
    else {
        next();
    }

    // En passant square

    skip_whitespace();
    if (cursor >= fen.size()) return std::nullopt;

    if (peek() != '-') {
        int file = next() - 'a';
        if (cursor >= fen.size()) return std::nullopt;

        int rank = next() - '0'; 
        if (rank > 7 || rank < 0) {
            return std::nullopt;
        }

        pos.en_passant_sq = rank * 8 + file;
    }
    else {
        next();
    }

    return pos;
}