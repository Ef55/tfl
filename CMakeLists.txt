cmake_minimum_required (VERSION 3.10)
project("Compiler")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_CXX_STANDARD 17)

find_package(Catch2 REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -pedantic -g")

add_subdirectory(tests)

# target_compile_options("Tests" PRIVATE -fsanitize=address)
# target_link_options("Tests" PRIVATE -fsanitize=address)
