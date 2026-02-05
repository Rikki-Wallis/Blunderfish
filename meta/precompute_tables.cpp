#include <cstdint>
#include <cstddef>
#include <unordered_set>
#include <bit>
#include <vector>
#include <cstdio>
#include <format>
#include <random>
#include <limits>

size_t get_index(uint64_t perm, uint64_t magic, size_t shift) {
    return static_cast<size_t>((perm * magic) >> shift);
}

template<typename...Args>
inline int fprint(FILE* stream, const std::format_string<Args...>& fmt, Args&&...args) {
    std::string str = std::format(fmt, std::forward<Args>(args)...);
    return fprintf(stream,"%s", str.c_str());
}

inline int fprint(FILE* stream, const std::format_string<>& fmt) {
    std::string str = std::format(fmt);
    return fprintf(stream, "%s", str.c_str());
}

// find a magic number that multiplies a permutation of a mask, such that it produces a unique top n bits per permutation, where n is the number of bits in the mask
uint64_t find_magic_number(uint64_t mask, size_t shift) {
    std::unordered_set<size_t> hit;

    std::random_device rd;
    std::mt19937_64 engine(rd()); 

    std::uniform_int_distribution<uint64_t> rand64(
        std::numeric_limits<uint64_t>::min(), // 0
        std::numeric_limits<uint64_t>::max()  // 18446744073709551615
    );

    for(;;) {
        // get a sparse random number
        uint64_t magic = rand64(engine) & rand64(engine) & rand64(engine);

        bool ok = true;
        uint64_t perm = mask;
        hit.clear();

        for(;;) {
            size_t index = get_index(perm, magic, shift);

            if (hit.contains(index)) { // there is a collision
                ok = false;
                break;
            }

            hit.insert(index);

            if (perm == 0) {
                break;
            }
        
            perm = (perm - 1) & mask;
        }

        if (ok) {
            return magic;
        }
    }
}

template<typename T>
static void dump_array(FILE* stream, const std::string& name, const char* type, const T* x, size_t count) {
    fprint(stream, "static const {} {}[] = {{\n", type, name);

    for (size_t i = 0; i < count; ++i) {
        if (i % 8 == 0) {
            fprint(stream, "    ");
        }

        fprint(stream, "0x{:x}, ", x[i]);

        if ((i+1) % 8 == 0) {
            fprint(stream, "\n");
        }
    }

    fprint(stream, "}};\n\n");
}

struct Tables {
    uint64_t              magic[64];
    size_t                shift[64];
    uint64_t              mask [64];
    std::vector<uint64_t> moves[64];

    void dump(FILE* stream, const char* name) const {
        dump_array(stream, std::format("{}_mask", name), "uint64_t", mask, std::size(mask));
        dump_array(stream, std::format("{}_magic", name), "uint64_t", magic, std::size(magic));
        dump_array(stream, std::format("{}_shift", name), "size_t", shift, std::size(shift));

        for (int i = 0; i < 64; ++i) {
            dump_array(stream, std::format("{}_move_buffer_{}", name, i), "uint64_t", moves[i].data(), moves[i].size());
        }

        fprint(stream, "static const uint64_t* {}_move[] = {{\n", name);
        for (int i = 0; i < 64; ++i) {
            fprint(stream, "    {}_move_buffer_{}, \n", name, i);
        }
        fprint(stream, "}};\n\n");
    }
};

static Tables generate_table(uint64_t(*mask_at)(size_t), uint64_t(*moves_at)(size_t,uint64_t)) {
    Tables table;

    for (size_t sq = 0; sq < 64; ++sq) {
        uint64_t mask = mask_at(sq);
        size_t bits = std::popcount(mask);
        size_t shift = 64 - bits;

        uint64_t magic = find_magic_number(mask, shift);

        table.moves[sq].resize(1ULL << bits);
        
        uint64_t perm = mask;
        
        for(;;) {
            size_t index = get_index(perm, magic, shift);

            table.moves[sq][index] = moves_at(sq, perm);

            if (perm == 0) {
                break;
            }

            perm = (perm - 1) & mask;
        }

        table.magic[sq] = magic;
        table.shift[sq] = shift;
        table.mask[sq]  = mask;
    }

    return table;
}

static uint64_t get_rank(size_t rank) {
    return (uint64_t)0xff << rank*8;
}

static uint64_t get_file(size_t file) {
    uint64_t ver_slice = (uint64_t)1 << file;
    uint64_t ver = 0;

    for(int i = 0; i < 8; ++i) {
        ver |= ver_slice << i*8;
    }

    return ver;
}

uint64_t rook_mask_at(size_t sq) {
    size_t file = sq % 8;
    size_t rank = sq / 8;

    uint64_t hor = get_rank(rank);
    uint64_t ver = get_file(file);

    // mask board edges
    hor &= ~(get_file(0) | get_file(7));
    ver &= ~(get_rank(0) | get_rank(7));

    uint64_t me = (uint64_t)1 << sq;

    return (hor | ver) & (~me);
}

uint64_t slide(size_t sq, uint64_t blockers, uint64_t(*translate)(uint64_t)) {
    uint64_t result = 0;

    uint64_t bb = (uint64_t)1 << sq;

    while (bb > 0) {
        bb = translate(bb);

        result |= bb;

        if (bb & blockers) {
            break;
        }
    }

    return result;
}

uint64_t bishop_moves_at(size_t sq, uint64_t blockers) {
    uint64_t result = 0;

    result |= slide(sq, blockers, [](uint64_t x){ return (x << 7) & (~get_file(7)); }); // left up
    result |= slide(sq, blockers, [](uint64_t x){ return (x << 9) & (~get_file(0)); }); // right up
    result |= slide(sq, blockers, [](uint64_t x){ return (x >> 7) & (~get_file(0)); }); // right down
    result |= slide(sq, blockers, [](uint64_t x){ return (x >> 9) & (~get_file(7)); }); // left down

    return result;
}

uint64_t bishop_mask_at(size_t sq) {
    uint64_t result = 0;

    result |= slide(sq, 0, [](uint64_t x){ return (x << 7) & (~get_file(7)); }) & (~(get_file(0) | get_rank(7))); // left up
    result |= slide(sq, 0, [](uint64_t x){ return (x << 9) & (~get_file(0)); }) & (~(get_file(7) | get_rank(7))); // right up
    result |= slide(sq, 0, [](uint64_t x){ return (x >> 7) & (~get_file(0)); }) & (~(get_file(7) | get_rank(0))); // right down
    result |= slide(sq, 0, [](uint64_t x){ return (x >> 9) & (~get_file(7)); }) & (~(get_file(0) | get_rank(0))); // left down

    return result;
}

uint64_t rook_moves_at(size_t sq, uint64_t blockers) {
    uint64_t result = 0;

    result |= slide(sq, blockers, [](uint64_t x){ return x << 8; }); // up
    result |= slide(sq, blockers, [](uint64_t x){ return x >> 8; }); // down
    result |= slide(sq, blockers, [](uint64_t x){ return (x << 1) & (~get_file(0)); }); // right
    result |= slide(sq, blockers, [](uint64_t x){ return (x >> 1) & (~get_file(7)); }); // left

    return result;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprint(stderr, "Usage %s <path>\n", argv[0]);
        return 1;
    }

    fprint(stderr, "Pre-computing tables...\n");

    Tables rooks = generate_table(rook_mask_at, rook_moves_at);
    Tables bishops = generate_table(bishop_mask_at, bishop_moves_at);

    const char* path = argv[1];
    FILE* file = nullptr;
    if (fopen_s(&file, path, "w") != 0) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return 1;
    }

    if (!file) {
        fprint(stderr, "Failed to write %s\n", path);
        return 1;
    }

    fprint(file, "#pragma once\n\n");
    fprint(file, "#include <cstdint>\n\n");

    rooks.dump(file, "rook");
    bishops.dump(file, "bishop");

    return 0;
}