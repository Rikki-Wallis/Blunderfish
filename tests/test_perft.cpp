#include <catch2/catch_test_macros.hpp>
#include "./src/blunderfish.h"
#include "./src/position.cpp"


TEST_CASE("Perft - Starting Position") {
    Position pos = decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    REQUIRE(perft_search(pos, 1) == 20);
    REQUIRE(perft_search(pos, 2) == 400);
    REQUIRE(perft_search(pos, 3) == 8902);
    REQUIRE(perft_search(pos, 4) == 197281);
    REQUIRE(perft_search(pos, 5) == 4865609);
    REQUIRE(perft_search(pos, 6) == 119080324);
    REQUIRE(perft_search(pos, 7) == 3195901860);
    REQUIRE(perft_search(pos, 8) == 84998978956);
    REQUIRE(perft_search(pos, 9) == 2439530234167);
    REQUIRE(perft_search(pos, 10) == 69352859712417);

}   