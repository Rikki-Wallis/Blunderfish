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

static const int* PST[NUM_PIECE_TYPES] = {
    nullptr,
    PAWN_PST,
    ROOK_PST,
    KNIGHT_PST,
    BISHOP_PST,
    QUEEN_PST,
    KING_PST,
};

int64_t Position::eval() const {
    int64_t value = sides[WHITE].material_value() - sides[BLACK].material_value();

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        for (int sq : set_bits(sides[WHITE].bb[p])) {
            value += PST[p][sq];
        }
    }

    for (int p = PIECE_PAWN; p < NUM_PIECE_TYPES; ++p) {
        for (int sq : set_bits(sides[BLACK].bb[p])) {
            value -= PST[p][sq^56];
        }
    }

    //int64_t sign = to_move == WHITE ? 1 : -1;

    return value;
}

void Position::increment_eval(Move& move) {
    // Define useful vars
    int opp_colour = opponent(move);
    int ally_colour = to_move;
    int from = move_from(move);
    int to = move_to(move);
    uint8_t moved_piece = piece_at[from];
    int from_pst_sq = move_side(move) == WHITE ? from : from ^ 56;
    int to_pst_sq   = move_side(move) == WHITE ? to : to ^ 56;

    // Remove material and update PST if capture has occured
    if (is_capture(move)) {
        int capture_sq = get_captured_square(move);
        Piece captured_piece = static_cast<Piece>(piece_at[capture_sq]);
        // White is - as we would be removing a white piece from the board
        int32_t captured_value = opp_colour == WHITE ? -piece_value_centipawns(captured_piece) : piece_value_centipawns(captured_piece);

        // Modify evaluation of material
        incremental_eval += captured_value;
        // Modify evaluation of PST table
        int pst_sq = ally_colour == WHITE ? capture_sq : capture_sq ^ 56;
        incremental_eval -= PST[captured_piece][pst_sq];
    }

    // DONT UNDERSTAND WHY THIS IMPROVED THE TESTS BUT IT DID
    if (ally_colour == WHITE) {
        std::printf("fired WHITE\n");
        // Remove moved piece from square from PST
        incremental_eval -= PST[moved_piece][from_pst_sq];
        // Add moved piece to square to PST
        incremental_eval += PST[move_end_piece(move)][to_pst_sq];
    } else {
        std::printf("fired BLACK\n");
        // Remove moved piece from square from PST
        incremental_eval += PST[moved_piece][from_pst_sq];
        // Add moved piece to square to PST
        incremental_eval -= PST[move_end_piece(move)][to_pst_sq];
    }

    // 2. update piece square tables (handle, ep, castling, promotions)
    if (move_type(move) & MOVE_SHORT_CASTLE) {
        int rook_old_pos = 7; 
        int rook_new_pos = 5;

        incremental_eval -= PST[PIECE_ROOK][rook_old_pos];
        incremental_eval += PST[PIECE_ROOK][rook_new_pos];

    } else if (move_type(move) & MOVE_LONG_CASTLE) {
        int rook_old_pos = 0;
        int rook_new_pos = 3;
        incremental_eval -= PST[PIECE_ROOK][rook_old_pos];
        incremental_eval += PST[PIECE_ROOK][rook_new_pos];
    }
}
