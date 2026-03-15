set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(GENERATED_HPP ${GENERATED_DIR}/generated_tables.h)
set(BOOK_HPP ${GENERATED_DIR}/opening_book.h)
set(BOOK_BIN ${CMAKE_CURRENT_LIST_DIR}/meta/Titans.bin)

add_executable(precompute_tables "meta/precompute_tables.cpp")
add_executable(bin_to_header "meta/bin_to_header.cpp")

set_target_properties(precompute_tables PROPERTIES
  MAP_IMPORTED_CONFIG_DEBUG Release
  MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
  MAP_IMPORTED_CONFIG_MINSIZEREL Release
)

add_custom_command(
    OUTPUT ${GENERATED_HPP} ${BOOK_HPP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND precompute_tables ${GENERATED_HPP}
    COMMAND bin_to_header ${BOOK_BIN} ${BOOK_HPP}
    DEPENDS precompute_tables bin_to_header ${BOOK_BIN}
    COMMAND_EXPAND_LISTS
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*engine_module\\.cpp$")

add_library(blunderfish STATIC ${SOURCES} ${GENERATED_HPP} ${BOOK_HPP})
target_include_directories(blunderfish PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(blunderfish PRIVATE ${GENERATED_DIR})