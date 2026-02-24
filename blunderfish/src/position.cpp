#include <cassert>
#include <cctype>

#include "blunderfish.h"
#include "common.h"

enum SideFlags {
    SIDE_FLAG_NONE = 0,
    SIDE_FLAG_CAN_CASTLE_KINGSIDE  = (1 << 0),
    SIDE_FLAG_CAN_CASTLE_QUEENSIDE = (1 << 1),
};


uint64_t Side::all() const {
    return bb[PIECE_PAWN] | bb[PIECE_ROOK] | bb[PIECE_KNIGHT] | bb[PIECE_BISHOP] | bb[PIECE_QUEEN] | bb[PIECE_KING];
}

void Position::display(bool display_metadata) const {
    verify_integrity();

    for (int rank=7; rank>=0; --rank) 
    {   
        print("{}|", rank+1);

        for (int file = 0; file < 8; ++file) 
        {
            int sq = rank * 8 + file;
            const char* piece = ".";
            uint64_t mask = sq_to_bb(sq);

            if      (sides[WHITE].bb[PIECE_PAWN]   & mask) piece = "P";
            else if (sides[WHITE].bb[PIECE_ROOK]   & mask) piece = "R";
            else if (sides[WHITE].bb[PIECE_KNIGHT] & mask) piece = "N";
            else if (sides[WHITE].bb[PIECE_BISHOP] & mask) piece = "B";
            else if (sides[WHITE].bb[PIECE_KING]   & mask) piece = "K";
            else if (sides[WHITE].bb[PIECE_QUEEN]  & mask) piece = "Q";

            if      (sides[BLACK].bb[PIECE_PAWN]   & mask) piece = "p";
            else if (sides[BLACK].bb[PIECE_ROOK]   & mask) piece = "r";
            else if (sides[BLACK].bb[PIECE_KNIGHT] & mask) piece = "n";
            else if (sides[BLACK].bb[PIECE_BISHOP] & mask) piece = "b";
            else if (sides[BLACK].bb[PIECE_KING]   & mask) piece = "k";
            else if (sides[BLACK].bb[PIECE_QUEEN]  & mask) piece = "q";

            print("{} ", piece);

        }

        print("\n");
    }

    print("  ---------------\n");
    print("  a b c d e f g h\n");

    if (display_metadata) {
        if (flags & POSITION_FLAG_WHITE_KCASTLE) {
            print("White can castle kingside.\n");
        }

        if (flags & POSITION_FLAG_WHITE_QCASTLE) {
            print("White can castle queenside.\n");
        }

        if (flags & POSITION_FLAG_BLACK_KCASTLE) {
            print("Black can castle kingside.\n");
        }

        if (flags & POSITION_FLAG_BLACK_QCASTLE) {
            print("Black can castle queenside.\n");
        }

        if (en_passant_sq != NULL_SQUARE) {
            auto [file, rank] = square_alg(en_passant_sq);
            print("En passant possible on {}{}.\n", file, rank);
        }
    }

    print("To move: {}\n", to_move == WHITE ? "white" : "black");
}

std::optional<Position> Position::decode_fen_string(const std::string& fen) {
    size_t cursor = 0;

    Position pos;

    auto set_piece = [&](int side, uint8_t piece, size_t rank, size_t file) {
        size_t index = rank * 8 + file;
        pos.sides[side].bb[piece] |= sq_to_bb(index);
        pos.piece_at[index] = piece; 
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
                    set_piece(side, PIECE_PAWN, rank, file);
                    file++;
                    break;
                case 'r':
                case 'R':
                    set_piece(side, PIECE_ROOK, rank, file);
                    file++;
                    break;
                case 'n':
                case 'N':
                    set_piece(side, PIECE_KNIGHT, rank, file);
                    file++;
                    break;
                case 'b':
                case 'B':
                    set_piece(side, PIECE_BISHOP, rank, file);
                    file++;
                    break;
                case 'q':
                case 'Q':
                    set_piece(side, PIECE_QUEEN, rank, file);
                    file++;
                    break;
                case 'k':
                case 'K':
                    set_piece(side, PIECE_KING, rank, file);
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

            switch(c) {
                default:
                    return std::nullopt;

                case 'k':
                    pos.flags |= POSITION_FLAG_BLACK_KCASTLE;
                    break;
                case 'K':
                    pos.flags |= POSITION_FLAG_WHITE_KCASTLE;
                    break;
                case 'q':
                    pos.flags |= POSITION_FLAG_BLACK_QCASTLE;
                    break;
                case 'Q':
                    pos.flags |= POSITION_FLAG_WHITE_QCASTLE;
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

    pos.initialise_zobrist();
    
    return pos;
}

uint64_t Position::all_pieces() const {
    return sides[0].all() | sides[1].all();
}

std::vector<Side> Position::get_sides() const {
    return std::vector<Side>(sides, sides + 2);
}

std::vector<uint64_t> Side::get_bbs() const {
    return std::vector<uint64_t>(bb, bb + NUM_PIECE_TYPES);
}

static void fill_map(uint8_t* map, uint64_t bb, uint8_t value) {
    assert(value);

    for (int i = 0; i < 64; ++i) {
        if (bb >> i & 1) {
            assert(!map[i]);
            map[i] = value;
        }
    }
}

void Position::verify_integrity() const {
    uint8_t map[64] = {};

    for (const Side& side : sides) {
        fill_map(map, side.bb[PIECE_PAWN],   PIECE_PAWN);
        fill_map(map, side.bb[PIECE_ROOK],   PIECE_ROOK);
        fill_map(map, side.bb[PIECE_KNIGHT], PIECE_KNIGHT);
        fill_map(map, side.bb[PIECE_BISHOP], PIECE_BISHOP);
        fill_map(map, side.bb[PIECE_QUEEN],  PIECE_QUEEN);
        fill_map(map, side.bb[PIECE_KING],   PIECE_KING);
    }

    assert(memcmp(map, piece_at, sizeof(map)) == 0);
}

int64_t Position::total_non_pawn_value() const {
    int64_t value = 0;

    for (int p = PIECE_PAWN + 1; p < NUM_PIECE_TYPES; ++p) {
        int count = std::popcount(sides[WHITE].bb[p]) + std::popcount(sides[BLACK].bb[p]);
        value += piece_value_centipawns((Piece)p) * count;
    }

    return value;
}