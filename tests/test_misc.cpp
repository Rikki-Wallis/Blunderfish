#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

#include "blunderfish.h"

static void test_capture_gen(Position& pos, int depth) {
    std::array<Move, 256> move_buf;
    std::array<Move, 256> capture_buf;

    std::span<Move> moves = pos.generate_moves(move_buf);
    std::span<Move> captures = pos.generate_captures(capture_buf);

    pos.filter_moves(moves);
    pos.filter_moves(captures);

    // recurse

    if (depth > 0) {
        for (Move mv : moves) {
            pos.make_move(mv);
            test_capture_gen(pos, depth-1);
            pos.unmake_move(mv);
        }
    }

    // remove non-captures

    for (int i = (int)moves.size()-1; i >= 0; --i) {
        Move mv = moves[i];

        if (!is_capture(mv)) {
            moves[i] = moves.back();
            moves = moves.subspan(0, moves.size()-1);
        }
    }

    // compare

    std::unordered_set move_set(moves.begin(), moves.end());
    std::unordered_set capture_set(captures.begin(), captures.end());

    REQUIRE(move_set == capture_set);
}

TEST_CASE("Generate-Captures Equals Captures from Generate-Moves") {
    auto pos = *Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    test_capture_gen(pos, 4); 
}

static void test_pin_case(const std::string& fen, const std::unordered_set<int>& white_pins, const std::unordered_set<int>& black_pins) {
    auto pos = *Position::decode_fen_string(fen);

    uint64_t white_mask = pos.generate_pin_mask(WHITE);
    uint64_t black_mask = pos.generate_pin_mask(BLACK);

    std::unordered_set<int> white_set{};
    std::unordered_set<int> black_set{};

    for (int x : set_bits(white_mask)) {
        white_set.insert(x);
    }

    for (int x : set_bits(black_mask)) {
        black_set.insert(x);
    }

    REQUIRE(white_set == white_pins);
    REQUIRE(black_set == black_pins);
}

TEST_CASE("Pin-mask is correct") {
    test_pin_case("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ", {33}, {29});
    test_pin_case("k1b4R/8/5r2/8/5P2/8/5K2/8 w - - 0 1", {29}, {58});
    test_pin_case("k1b4R/8/5r2/5b2/5P2/8/5K2/8 w - - 0 1", {}, {58});
    test_pin_case("k1b3bR/8/5r2/5b2/5P2/8/5K2/8 w - - 0 1", {}, {});
    test_pin_case("k1b3bR/8/5r2/5b2/5P2/5r2/5K2/8 w - - 0 1", {}, {});
    test_pin_case("k1b3bR/8/1q3r2/5b2/3B1P2/5r2/5K2/8 w - - 0 1", {27}, {});
    test_pin_case("k1b3bR/8/1q3r2/5b2/3B1P2/4Pr2/5K2/8 w - - 0 1", {}, {});
    test_pin_case("k1b3bR/1p6/1q3r2/5b2/3BBP2/4Pr2/5K2/8 w - - 0 1", {}, {49});
    test_pin_case("k1b3bR/1p6/1q3r2/r4b2/3BBP2/4Pr2/5K2/R7 w - - 0 1", {}, {49, 32});
}