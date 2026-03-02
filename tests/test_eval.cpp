#include <catch2/catch_test_macros.hpp>
#include "blunderfish.h"

void eval_search(int depth, Position& position) {

    if (depth == 0) {
        return;
    } else {
        std::array<Move, 256> move_buffer;
        std::span<Move> moves = position.generate_moves(move_buffer);

        for (Move move : moves) {
            position.make_move(move);
            position.verify_integrity();

            if (position.eval() != position.incremental_eval) {
                position.display();
            }

            REQUIRE(position.eval() == position.incremental_eval);

            if (!position.is_in_check(position.to_move)) {
                eval_search(depth-1, position);
            }
            
            position.unmake_move(move);
            position.verify_integrity();
        }
    }
}

TEST_CASE("Eval - increment_eval equals eval | STARTING POSITION") {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    eval_search(5, pos);
}

TEST_CASE("Eval - increment_eval equals eval | KIWIPETE_POSITION") {
    Position pos = *Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    eval_search(5, pos);
}