#pragma once

#include <cstdint>
#include <string>

enum {
    WHITE,
    BLACK
};

struct Side {
    uint64_t pawns;
    uint64_t rooks;
    uint64_t knights;
    uint64_t bishops;
    uint64_t queens;
    uint64_t king;
};

struct Position {
    Side sides[2];
    int to_move;

    void display() const;
    static Position decode_fen_string(const std::string& fen);
};