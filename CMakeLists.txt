cmake_minimum_required(VERSION 3.16)
project(Blunderfish LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(GENERATED_HPP ${GENERATED_DIR}/generated.h)

add_custom_command(
    OUTPUT ${GENERATED_HPP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND Python3::Interpreter
            ${CMAKE_SOURCE_DIR}/meta/precompute_tables.py
            ${GENERATED_HPP}
    DEPENDS
        ${CMAKE_SOURCE_DIR}/meta/precompute_tables.py
    COMMENT "Pre-computing tables..."
    VERBATIM
)

add_custom_target(precompute
    DEPENDS ${GENERATED_HPP}
)

# Compiler warning flags
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wimplicit-int-conversion)
endif()

# Gather sources recursively from src/
file(GLOB_RECURSE SOURCES
  CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
add_dependencies(${PROJECT_NAME} precompute)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Ensure target-level warnings-as-errors / warning level
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wimplicit-int-conversion)
endif()
