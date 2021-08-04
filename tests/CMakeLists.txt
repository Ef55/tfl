SET(TEST_SRC
    "main.cpp"
    "lexer/Lexer.cpp"
    "lexer/Regex.cpp"
    "lexer/RegexDerivation.cpp"
)

add_executable("Tests" ${TEST_SRC})
target_include_directories("Tests" PRIVATE "../include/")
target_link_libraries("Tests" PRIVATE Catch2::Catch2)
