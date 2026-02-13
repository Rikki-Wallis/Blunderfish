add_executable(test_perft test_perft.cpp)
target_link_libraries(test_perft PRIVATE blunderfish Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(
  test_perft
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)