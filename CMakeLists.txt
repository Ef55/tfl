cmake_minimum_required (VERSION 3.10)
project("Compiler")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Catch2 REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(tests)
add_subdirectory(examples)
