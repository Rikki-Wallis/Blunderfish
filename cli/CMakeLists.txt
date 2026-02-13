add_executable(cli main.cpp)
target_include_directories(cli PRIVATE ${CMAKE_SOURCE_DIR}/blunderfish/src)
target_link_libraries(cli PRIVATE blunderfish)