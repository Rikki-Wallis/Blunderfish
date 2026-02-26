#include <catch2/catch_test_macros.hpp>
#include "blunderfish.h"
#include <array>


void zobrist_search(int depth, Position& position) {
    uint64_t incremental_hash = position.zobrist;
    position.create_zobrist();
    uint64_t new_hash = position.zobrist;

    if (new_hash != incremental_hash) {
        position.display();
    }
    REQUIRE(new_hash == incremental_hash);

    if (depth == 0) {
        return;
    } else {
        
        std::array<Move, 256> move_buffer;
        std::span<Move> moves = position.generate_moves(move_buffer);

        for (Move move : moves) {
            position.make_move(move);
            position.verify_integrity();
            
            zobrist_search(depth-1, position);
            
            position.unmake_move(move);
            position.verify_integrity();
        }
    }
}

TEST_CASE("Zobrist - make_move equals intialise_zobrist | STARTING POSITION") {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    zobrist_search(5, pos);
}

TEST_CASE("Zobrist - make_move equals intialise_zobrist | KIWIPETE POSITION") {
    Position pos = *Position::decode_fen_string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    zobrist_search(5, pos);
}