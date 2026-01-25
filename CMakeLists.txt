cmake_minimum_required(VERSION 3.16)
project(Blunderfish LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warning flags
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wimplicit-int-conversion)
endif()

set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(GENERATED_HPP ${GENERATED_DIR}/generated_tables.h)

add_executable(precompute_tables "meta/precompute_tables.cpp")

if(MSVC)
  target_compile_options(precompute_tables PRIVATE /O2)
else()
  target_compile_options(precompute_tables PRIVATE -O3 -march=native)
endif()
target_compile_definitions(precompute_tables PRIVATE NDEBUG)

add_custom_command(
    OUTPUT ${GENERATED_HPP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND precompute_tables
            ${GENERATED_HPP}
    DEPENDS precompute_tables
    COMMAND_EXPAND_LISTS
)

# Gather sources recursively from src/
file(GLOB_RECURSE SOURCES
  CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${GENERATED_HPP})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)