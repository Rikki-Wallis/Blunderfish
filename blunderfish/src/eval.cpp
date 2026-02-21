#include "blunderfish.h"

int32_t piece_value_centipawns(Piece piece) {
    switch (piece) {
        default:
            assert(false);
            return 0;
        case PIECE_NONE:
            return 0;
        case PIECE_PAWN:
            return 100;
        case PIECE_KNIGHT:
            return 300;
        case PIECE_BISHOP:
            return 300;
        case PIECE_ROOK:
            return 500;
        case PIECE_KING:
            return 0;
        case PIECE_QUEEN:
            return 900;
    }
}

int64_t Side::material_value() const {
    int64_t sum = 0;

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        int n = std::popcount(bb[p]);
        sum += n * piece_value_centipawns((Piece)p);
    }

    return sum;
}

int64_t Position::eval() const {
    int64_t value = sides[WHITE].material_value() - sides[BLACK].material_value();
    int64_t sign = to_move == WHITE ? 1 : -1;
    return value * sign;
}