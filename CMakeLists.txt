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

add_subdirectory(blunderfish)
add_subdirectory(cli)