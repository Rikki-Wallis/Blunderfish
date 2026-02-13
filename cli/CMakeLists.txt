add_executable(cli 
    main.cpp 
    ../tests/test_perft.cpp
)
target_link_libraries(cli PRIVATE 
    blunderfish 
    Catch2::Catch2
)