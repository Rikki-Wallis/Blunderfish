#pragma once

#include <cstdint>
#include <string>
#include <optional>

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
    int metadata;

    void set_can_castle_kingside(bool);
    void set_can_castle_queenside(bool);

    bool can_castle_kingside() const;
    bool can_castle_queenside() const;
};

struct Position {
    Side sides[2];
    int to_move;
    std::optional<int> en_passant_sq;

    void display(bool display_metadata=false) const;
    static std::optional<Position> decode_fen_string(const std::string& fen);
};