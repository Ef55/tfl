#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <numeric>

#include "tfl/Lexer.hpp"
#include "tfl/Parser.hpp"

enum class Special {
    OP_PAR,
    CL_PAR,
    SPACE,
    NEWLINE
};

using Token = std::variant<int, char, Special>;

static class: tfl::Regexes<char> {
    using Regex = tfl::Regex<char>;

    Regex const num     = range('0', '9');
    Regex const op      = any_of({'+', '-', '*', '/'});
    Regex const space   = any_of({'\t', '\n', '\v', '\f', '\r', ' '});
    Regex const newline = opt(literal('\r')) & literal('\n');

    tfl::Lexer<char, Token, std::string> const lexer = tfl::Lexer<char, Token, std::string>::make({
        {*newline,      [](auto){   return Special::NEWLINE; }},
        {num & *num,    [](auto w){ return std::stoi(w); }},
        {*space,        [](auto){   return Special::SPACE; }},
        {op,            [](auto s){ return s[0]; }},
        {literal('('),  [](auto){   return Special::OP_PAR; }},
        {literal(')'),  [](auto){   return Special::CL_PAR; }},
    }).map([](auto v){ return v.value(); })
      .filter([](auto v){ return v != Token{Special::SPACE}; });

public:
    std::vector<Token> operator()(std::string const& input) const {
        return lexer(input.cbegin(), input.cend());
    }
} lexer;


static class: tfl::Parsers<Token> {
    template<typename R>
    using Parser = tfl::Parser<Token, R>;

    template<typename R>
    Parser<R> token() {
        return elem([](auto v){ return std::holds_alternative<R>(v); })
            .map([](auto v){ return std::get<R>(v); });
    }

    template<typename R>
    Parser<R> token(R const& r) {
        return elem([r](auto v){ return std::holds_alternative<R>(v) && v == Token{r}; })
            .map([](auto v){ return std::get<R>(v); });
    }

    Parser<int> expression_parser() {
        Parser<int> number = token<int>();
        Parser<char> plus_minus = token('+') | token('-');
        Parser<char> time_div = token('*') | token('/');

        Parser<Special> op_par = token(Special::OP_PAR);
        Parser<Special> cl_par = token(Special::CL_PAR);

        auto expression = recursive<int>();

        Parser<int> literal = number | 
            (token('-') & number).map([](auto p){ return -p.second; }) |
            (op_par & expression & cl_par).map([](auto p){ return p.first.second; });

        Parser<int> prod = (literal & many(time_div & literal)).map([](auto p) {
            return std::accumulate(p.second.cbegin(), p.second.cend(), p.first, [](auto i, auto op) {
                return op.first == '*' ? i*op.second : i/op.second;
            });
        });

        Parser<int> sum = (prod & many(plus_minus & prod)).map([](auto p) {
            return std::accumulate(p.second.cbegin(), p.second.cend(), p.first, [](auto i, auto op) {
                return op.first == '+' ? i+op.second : i-op.second;
            });
        });

        expression = sum;

        return expression;
    }

    Parser<int> parser = expression_parser();

public:
    using It = std::vector<Token>::const_iterator;

    auto operator()(It beg, It end) const {
        return parser(beg, end);
    }
} const parser;


int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Please provide a file to parse." << std::endl;
        return -1;
    }

    std::ifstream file(*++argv);
    if(!file) {
        std::cerr << "File either not found or empty." << std::endl;
        return -1;
    }

    std::string input((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    file.close();

    auto tokens = lexer(input);
    auto end = tokens.cend();

    auto cur = tokens.cbegin();
    for(
        auto next = std::find(cur, end, Token{Special::NEWLINE}); 
        cur != end+1; 
        cur = ++next
    ) {
        next = std::find(cur, end, Token{Special::NEWLINE});
        if(cur != next) // In case of final blank line
            std::cout << parser(cur, next) << std::endl;
    }

    return 0;
}