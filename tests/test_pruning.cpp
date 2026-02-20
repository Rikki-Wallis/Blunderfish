#include <catch2/catch_test_macros.hpp>

#include "blunderfish.h"

TEST_CASE("Pruned Negamax Equals Plain Negamax on start") {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    for (int depth = 1; depth <= 6; ++depth) {
        int64_t a = pos.negamax(depth, 1);
        int64_t b = pos.pruned_negamax(depth, 1, INT32_MIN, INT32_MAX);
        REQUIRE(a == b);
    }
}