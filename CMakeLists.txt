cmake_minimum_required (VERSION 3.13)
project("TFL")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(tests)
add_subdirectory(benchmarks)
add_subdirectory(examples)

find_package(Doxygen)
if(DOXYGEN_FOUND) 
    set(DOXYGEN_STRIP_FROM_PATH "include")
    doxygen_add_docs(
        "Doc" 
        "include/tfl"
    )
endif()