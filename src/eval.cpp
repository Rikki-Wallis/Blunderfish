#include "blunderfish.h"

inline int piece_value(Piece piece) {
    switch (piece) {
        default:
            assert(false);
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

int Side::material_value() const {
    int sum = 0;

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        int n = std::popcount(bb[p]);
        sum += n * piece_value((Piece)p);
    }

    return sum;
}

int Position::eval() const {
    return sides[WHITE].material_value() - sides[BLACK].material_value();
}