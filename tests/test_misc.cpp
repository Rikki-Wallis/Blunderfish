#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

#include "blunderfish.h"

static void test_capture_gen(const std::string& FEN) {
    Position pos = *Position::decode_fen_string(FEN);

    std::array<Move, 256> move_buf;
    std::array<Move, 256> capture_buf;

    std::span<Move> moves = pos.generate_moves(move_buf);
    std::span<Move> captures = pos.generate_captures(capture_buf);

    pos.filter_moves(moves);
    pos.filter_moves(captures);

    for (int i = (int)moves.size()-1; i >= 0; --i) {
        Move mv = moves[i];

        int square = get_captured_square(mv);

        if (pos.piece_at[square] == PIECE_NONE) {
            moves[i] = moves.back();
            moves = moves.subspan(0, moves.size()-1);
        }
    }

    std::unordered_set move_set(moves.begin(), moves.end());
    std::unordered_set capture_set(captures.begin(), captures.end());

    REQUIRE(move_set.size() > 0);
    REQUIRE(move_set == capture_set);
}

TEST_CASE("Generate-Captures Equals Captures from Generate-Moves") {
    test_capture_gen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); 
}

TEST_CASE("Pruned Negamax Equals Plain Negamax on start") {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    for (int depth = 1; depth <= 6; ++depth) {
        int64_t a = pos.negamax(depth, 1);

        KillerTable killers{};
        HistoryTable history{};
        TranspositionTable tt(TRANSPOSITION_TABLE_SIZE);
        int64_t b = pos.pruned_negamax(depth, tt, history, killers, 1, true, INT32_MIN, INT32_MAX);
        REQUIRE(a == b);
    }
}