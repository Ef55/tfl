#include <iostream>
#include <fstream>

#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"


int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Please provide a file to compile." << std::endl;
        return -1;
    }

    std::ifstream file(*++argv);
    if(!file) {
        std::cerr << "File either not found or empty." << std::endl;
        return -1;
    }

    std::string input((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    file.close();

    std::cout << "Input:\n" << input << std::endl;

    std::cout << "Tokens:\n";
    auto tokens = lex(input);
    for(auto token: tokens) {
        std::cout << token << std::flush;
    }
    std::cout << std::endl;
    

    std::cout << "Json:\n";
    auto json = parse(tokens);
    std::cout << json << std::endl;

    return 0;
}