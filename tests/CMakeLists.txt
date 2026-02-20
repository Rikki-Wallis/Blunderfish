
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2   # Use a specific version
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_perft test_perft.cpp)
target_link_libraries(test_perft PRIVATE blunderfish Catch2::Catch2WithMain)
add_executable(test_pruning test_pruning.cpp)
target_link_libraries(test_pruning PRIVATE blunderfish Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(test_perft)
catch_discover_tests(test_pruning)