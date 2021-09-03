set(BENCHMARK_SRC
    "Regex.cpp"
    "DFAOptimizations.cpp"
)

find_package(Catch2 3 REQUIRED)

add_executable("Benchmarks" ${BENCHMARK_SRC})
target_include_directories("Benchmarks" PRIVATE "../include/")
target_link_libraries("Benchmarks" PRIVATE Catch2::Catch2WithMain)

target_compile_options("Benchmarks" PRIVATE -Wall)
target_compile_options("Benchmarks" PRIVATE -pedantic)