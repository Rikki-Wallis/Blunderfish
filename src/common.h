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