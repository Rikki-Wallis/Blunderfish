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

// stolen from https://www.chessprogramming.org/Simplified_Evaluation_Function

static const int PAWN_PST[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0,
};

static const int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

static const int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

static const int ROOK_PST[64] = {
    0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    5, 10, 10, 10, 10, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0,
};

static const int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

static const int KING_PST[64] = {
    20, 30, 10,  0,  0, 10, 30, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

static const int ZEROED_PST[64] = {};

static const int* PST[NUM_PIECE_TYPES] = {
    ZEROED_PST,
    PAWN_PST,
    ROOK_PST,
    KNIGHT_PST,
    BISHOP_PST,
    QUEEN_PST,
    KING_PST,
};

int32_t unsigned_pst_value(Piece piece, int square, int side) {
    return PST[piece][square ^ (bool_to_mask<int>(side==BLACK) & 56)];
}

int64_t Position::compute_eval() const {
    int64_t value = sides[WHITE].material_value() - sides[BLACK].material_value();

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        for (int sq : set_bits(sides[WHITE].bb[p])) {
            value += unsigned_pst_value((Piece)p, sq, WHITE);
        }
    }

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        for (int sq : set_bits(sides[BLACK].bb[p])) {
            value -= unsigned_pst_value((Piece)p, sq, BLACK);
        }
    }

    return value;
}

int64_t Position::signed_eval() const {
    int64_t sign = to_move == WHITE ? 1 : -1;
    return incremental_eval * sign;
}
 
void Position::eval_remove_piece(Piece piece, int sq, int side) {
    int32_t sign = side == BLACK ? -1 : 1;
    incremental_eval -= sign * unsigned_pst_value(piece, sq, side);
    incremental_eval -= sign * piece_value_centipawns(piece);
}

void Position::eval_add_piece(Piece piece, int sq, int side) {
    int32_t sign = side == BLACK ? -1 : 1;
    incremental_eval += sign * unsigned_pst_value(piece, sq, side);
    incremental_eval += sign * piece_value_centipawns(piece);
}