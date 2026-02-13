#include <catch2/catch_test_macros.hpp>

#include "blunderfish.h"
#include <chrono>
#include <cstdio>
#include <iostream>

static void run_and_time(int depth, Position& pos) {
    // Time perft search
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t result = perft_search(depth, pos);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;

    // Calculate nodes per second
    double nps = seconds > 0 ? result / seconds : 0;

    std::cout << "Depth " << depth << ": " << result << " nodes in "
              << seconds << "s (" << static_cast<uint64_t>(nps) << " nodes/sec)\n";
}

TEST_CASE("Perft - Benchmark") {
    auto maybe_pos = Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
    } else {

        Position pos(std::move(*maybe_pos));

        run_and_time(1, pos);
        run_and_time(2, pos);
        run_and_time(3, pos);
        run_and_time(4, pos);
        run_and_time(5, pos);
        run_and_time(6, pos);
    }
}

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
    }
}

TEST_CASE("Perft - Kiwipete Position") {    
    auto maybe_pos = Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
    } else {

    Position pos(std::move(*maybe_pos));

    REQUIRE(perft_search(1, pos) == 48);
    REQUIRE(perft_search(2, pos) == 2039);
    REQUIRE(perft_search(3, pos) == 97862);
    REQUIRE(perft_search(4, pos) == 4085603);
    REQUIRE(perft_search(5, pos) == 193690690);
    REQUIRE(perft_search(6, pos) == 8031647685);
    }
}