
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2   # Use a specific version
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_perft test_perft.cpp)
target_link_libraries(test_perft PRIVATE blunderfish Catch2::Catch2WithMain)
add_executable(test_misc test_misc.cpp)
target_link_libraries(test_misc PRIVATE blunderfish Catch2::Catch2WithMain)
add_executable(test_zobrist test_zobrist.cpp)
target_link_libraries(test_zobrist PRIVATE blunderfish Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(test_perft)
catch_discover_tests(test_misc)
catch_discover_tests(test_zobrist)