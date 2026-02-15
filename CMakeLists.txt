cmake_minimum_required(VERSION 3.16)
project(Blunderfish LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warning flags
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wimplicit-int-conversion)
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_subdirectory(blunderfish)
add_subdirectory(cli)
add_subdirectory(tests)