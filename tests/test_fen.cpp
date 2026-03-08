#include <catch2/catch_test_macros.hpp>

#include "blunderfish.h"

static void test_fen(Position& pos, int depth) {
    std::string fen = pos.fen();
    auto maybe_pos = Position::decode_fen_string(fen);

    REQUIRE(maybe_pos.has_value());
    auto pos2 = std::move(*maybe_pos);

    REQUIRE(pos2.zobrist == pos.zobrist);

    if (depth == 0) {
        return;
    }

    std::array<Move, 256> move_buf;
    std::span<Move> moves = pos.generate_moves(move_buf);

    int my_side = pos.to_move;

    for (Move mv : moves) {
        pos.make_move(mv);

        if (!pos.is_checked[my_side]) {
            test_fen(pos, depth-1);
        }

        pos.unmake_move(mv);
    }
}

TEST_CASE("FEN encode and decode is correct") {
    Position pos = *Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    test_fen(pos, 3);
}