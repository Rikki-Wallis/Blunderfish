#include <catch2/catch_test_macros.hpp>
#include "blunderfish.h"

void eval_search(int depth, Position& position, Move prev_move) {
    (void)prev_move;
    int64_t new_eval = position.compute_eval();

    if (new_eval != position.incremental_eval) {
        position.display();
    }

    REQUIRE(new_eval == position.incremental_eval);

    int my_side = position.to_move;

    if (depth == 0) {
        return;
    } else {
        std::array<Move, 256> move_buffer;
        std::span<Move> moves = position.generate_moves(move_buffer);

        for (Move move : moves) {
            position.make_move(move);
            position.verify_integrity();

            if (!position.is_in_check(my_side)) {
                eval_search(depth-1, position, move);
            }
            
            position.unmake_move(move);
            position.verify_integrity();
        }
    }
}

TEST_CASE("Eval - increment_eval equals eval | STARTING POSITION") {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    eval_search(5, pos, NULL_MOVE);
}

TEST_CASE("Eval - increment_eval equals eval | KIWIPETE_POSITION") {
    Position pos = *Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    eval_search(5, pos, NULL_MOVE);
}