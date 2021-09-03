add_executable("Graphs" "main.cpp")
target_include_directories("Graphs" PRIVATE "../../include/")

target_compile_options("Graphs" PRIVATE -g)
target_compile_options("Graphs" PRIVATE -Wall)
target_compile_options("Graphs" PRIVATE -pedantic)

target_compile_options("Graphs" PRIVATE -fsanitize=address)
target_link_options("Graphs" PRIVATE -fsanitize=address)
