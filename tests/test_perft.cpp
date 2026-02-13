#include <catch2/catch_test_macros.hpp>
#include "blunderfish.h"


TEST_CASE("Perft - Starting Position") {
    auto maybe_pos = Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
    } else {

    Position pos(std::move(*maybe_pos));

    REQUIRE(perft_search(1, pos) == 20);
    REQUIRE(perft_search(2, pos) == 400);
    REQUIRE(perft_search(3, pos) == 8902);
    REQUIRE(perft_search(4, pos) == 197281);
    REQUIRE(perft_search(5, pos) == 4865609);
    REQUIRE(perft_search(6, pos) == 119080324);
    REQUIRE(perft_search(7, pos) == 3195901860);
    REQUIRE(perft_search(8, pos) == 84998978956);
    REQUIRE(perft_search(9, pos) == 2439530234167);
    REQUIRE(perft_search(10, pos) == 69352859712417);
    }
}   