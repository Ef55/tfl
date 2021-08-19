#include <iostream>
#include <fstream>

#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Codegen.hpp"

#include <llvm/Support/raw_os_ostream.h>

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

    auto tokens = lex(input);

    std::cout << "Tokens:\n";
    for(auto t : tokens) {
        std::cout << t << ' ' << std::flush;
    }
    std::cout << std::endl;

    auto asts = parse(tokens);


    std::cout << "\nLLVM's IR:\n";
    CodeGenerator codegen;
    for(auto ast: asts) {
        codegen(ast);
    }

    llvm::raw_os_ostream out(std::cout);
    out << codegen.code() << '\n';

    return 0;
}