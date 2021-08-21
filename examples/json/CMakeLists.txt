set(SRC
    "src/main.cpp"
    "src/Json.cpp"
    "src/Token.cpp"
    "src/Lexer.cpp"
    "src/Parser.cpp"
)

add_executable("Json" ${SRC})
target_include_directories("Json" PRIVATE "../../include/")

target_compile_options("Json" PRIVATE -g)
target_compile_options("Json" PRIVATE -Wall)
target_compile_options("Json" PRIVATE -pedantic)

target_compile_options("Json" PRIVATE -fsanitize=address)
target_link_options("Json" PRIVATE -fsanitize=address)
