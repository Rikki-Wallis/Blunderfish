#pragma once

#include <cassert>
#include <format>
#include <string>
#include <cstdio>

template<typename...Args>
int print(const std::format_string<Args...>& fmt, Args&&... args) {
    std::string str = std::format(fmt, std::forward<Args>(args)...);
    return printf("%s", str.c_str());
}

int print(const std::format_string<>& fmt) {
    std::string str = std::format(fmt);
    return printf("%s", str.c_str());
}