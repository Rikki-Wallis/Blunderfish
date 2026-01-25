#pragma once

#include <cassert>
#include <format>
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstring>

template<typename...Args>
inline int print(const std::format_string<Args...>& fmt, Args&&... args) {
    std::string str = std::format(fmt, std::forward<Args>(args)...);
    return printf("%s", str.c_str());
}

inline int print(const std::format_string<>& fmt) {
    std::string str = std::format(fmt);
    return printf("%s", str.c_str());
}