set(TEST_SRC
    "lexer/Lexer.cpp"
    "lexer/Regex.cpp"
    "lexer/RegexNullability.cpp"
    "lexer/RegexMetrics.cpp"
    "lexer/RegexPrinter.cpp"
    "lexer/RegexAlphabet.cpp"
    "lexer/DFA.cpp"
    "lexer/NFA.cpp"
    "parser/Syntax.cpp"
    "utils/InputBuffer.cpp"
    "utils/Lazy.cpp"
)

find_package(Catch2 3 REQUIRED)

add_executable("Tests" ${TEST_SRC})
target_include_directories("Tests" PRIVATE "../include/")
target_link_libraries("Tests" PRIVATE Catch2::Catch2WithMain)

target_compile_options("Tests" PRIVATE -g)
target_compile_options("Tests" PRIVATE -Wall)
target_compile_options("Tests" PRIVATE -pedantic)

target_compile_options("Tests" PRIVATE -fsanitize=address)
target_link_options("Tests" PRIVATE -fsanitize=address)
