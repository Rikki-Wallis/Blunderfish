#pragma once

#include <bit>
#include <cassert>
#include <format>
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <span>

template<typename...Args>
inline int print(const std::format_string<Args...>& fmt, Args&&... args) {
    std::string str = std::format(fmt, std::forward<Args>(args)...);
    return printf("%s", str.c_str());
}

inline int print(const std::format_string<>& fmt) {
    std::string str = std::format(fmt);
    return printf("%s", str.c_str());
}

struct set_bits {
    uint64_t x;

    set_bits(uint64_t x)
        : x(x)
    {}

    struct Iterator {
        uint64_t v;

        bool operator!=(std::default_sentinel_t) const {
            return v != 0;
        }

        Iterator& operator++() {
            v &= v - 1;
            return *this;
        }

        uint8_t operator*() const {
            return (uint8_t)std::countr_zero(v);
        }
    };

    Iterator begin() const { return { .v = x }; }
    std::default_sentinel_t end() const { return {}; }
};

template<size_t N>
class Bitset {
public:
    Bitset() {
        memset(_data, 0, sizeof(_data));
    }

    bool get(size_t index) const {
        assert(index < N);
        return ((_data[index / 64] >> (index % 64)) & 1) != 0;
    } 

    void set(size_t index) {
        assert(index < N);
        _data[index / 64] |= uint64_t(1) << (index % 64);
    } 

    void unset(size_t index) {
        assert(index < N);
        _data[index / 64] &= ~(uint64_t(1) << (index % 64));
    } 

private:
    static constexpr size_t NUM_WORDS = (N + 63) / 64;
    uint64_t _data[NUM_WORDS];
};