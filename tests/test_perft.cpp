#include <catch2/catch_test_macros.hpp>

#include "blunderfish.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <vector>
#include <array>

template <typename Func>
static uint64_t run_and_time(Func&& func) {
    // Time perft search
    auto start = std::chrono::high_resolution_clock::now();
    std::forward<Func>(func)(); 
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    uint64_t ms = duration.count();

    return ms;
}

TEST_CASE("Perft - Benchmark") {
    auto maybe_pos = Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
    } else {

        Position pos(std::move(*maybe_pos));

        std::vector<uint64_t> times;
        times.push_back(run_and_time([&] {return perft_search(1, pos);}));
        times.push_back(run_and_time([&] {return perft_search(2, pos);}));
        times.push_back(run_and_time([&] {return perft_search(3, pos);}));
        times.push_back(run_and_time([&] {return perft_search(4, pos);}));
        times.push_back(run_and_time([&] {return perft_search(5, pos);}));
        times.push_back(run_and_time([&] {return perft_search(6, pos);}));

        size_t i = 0;
        std::vector<uint64_t> nodes = {20, 400, 8902, 197281, 4865609, 119060324};
        for (i = 0; i < times.size(); ++i) {
                        
            double seconds = times[i] / 1000.0;
            double nps = seconds > 0 ? nodes[i] / seconds : 0;
            std::cout << "Depth " << i+1 << ": " << nodes[i] << " nodes in "
                      << seconds << "s (" << nps << " nodes/sec)\n";
            
        };
    }
}

TEST_CASE("best_move - Benchmark") {

    auto maybe_pos = Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    if (!maybe_pos) {
        printf("Invalid FEN string.\n");
    } else {

        Position pos(std::move(*maybe_pos));

        std::vector<uint64_t> times;
        std::array<Move, 256> move_buf;
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 1);}));
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 2);}));
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 3);}));
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 4);}));
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 5);}));
        times.push_back(run_and_time([&] {return pos.best_move(pos.generate_moves(move_buf), 6);}));

        size_t i = 0;
        std::vector<uint64_t> nodes = {20, 400, 8902, 197281, 4865609, 119060324};
        for (i = 0; i < times.size(); ++i) {
                        
            double seconds = times[i] / 1000.0;
            double nps = seconds > 0 ? nodes[i] / seconds : 0;
            std::cout << "Depth " << i+1 << ": " << nodes[i] << " nodes in "
                      << seconds << "s (" << nps << " nodes/sec)\n";
            
        };
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
        REQUIRE(perft_search(6, pos) == 119060324);
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
        //REQUIRE(perft_search(6, pos) == 8031647685);
    }
}