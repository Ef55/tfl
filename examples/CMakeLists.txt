add_executable("Calculator" "Calculator.cpp")
target_include_directories("Calculator" PRIVATE "../include/")

target_compile_options("Calculator" PRIVATE -g)
target_compile_options("Calculator" PRIVATE -Wall)
target_compile_options("Calculator" PRIVATE -pedantic)

target_compile_options("Calculator" PRIVATE -fsanitize=address)
target_link_options("Calculator" PRIVATE -fsanitize=address)
