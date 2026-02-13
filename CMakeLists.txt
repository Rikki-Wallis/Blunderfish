cmake_minimum_required(VERSION 3.16)
project(Blunderfish LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include catch2
#include(FetchContent)

#FetchContent_Declare(
#  catch2
#  GIT_REPOSITORY http://github.com/catchorg/Catch2
#  GIT_TAG v3.5.4
#)

#FetchContent_MakeAvailable(Catch2)

# Compiler warning flags
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wimplicit-int-conversion)
endif()

set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(GENERATED_HPP ${GENERATED_DIR}/generated_tables.h)

add_executable(precompute_tables "meta/precompute_tables.cpp")
#add_executable(tests tests/test_perft.cpp)

#target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

set_target_properties(precompute_tables PROPERTIES
  MAP_IMPORTED_CONFIG_DEBUG Release
  MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
  MAP_IMPORTED_CONFIG_MINSIZEREL Release
)

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

# Remove engine_module.cpp from the list
list(FILTER SOURCES EXCLUDE REGEX ".*engine_module\\.cpp$")

add_executable(${PROJECT_NAME} ${SOURCES} ${GENERATED_HPP})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
