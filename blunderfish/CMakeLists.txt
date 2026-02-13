set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)
set(GENERATED_HPP ${GENERATED_DIR}/generated_tables.h)

add_executable(precompute_tables "meta/precompute_tables.cpp")

set_target_properties(precompute_tables PROPERTIES
  MAP_IMPORTED_CONFIG_DEBUG Release
  MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
  MAP_IMPORTED_CONFIG_MINSIZEREL Release
)

add_custom_command(
    OUTPUT ${GENERATED_HPP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND precompute_tables ${GENERATED_HPP}
    DEPENDS precompute_tables
    COMMAND_EXPAND_LISTS
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*engine_module\\.cpp$")

add_library(blunderfish STATIC ${SOURCES} ${GENERATED_HPP})
target_include_directories(blunderfish PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(blunderfish PRIVATE ${GENERATED_DIR})