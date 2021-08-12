#include <iostream>
#include <string>
#include <variant>
#include <numeric>

#include "tfl/Lexer.hpp"
#include "tfl/Parser.hpp"

enum class Special {
    PLUS,
    TIME,
    SPACE
};

using Token = std::variant<int, float, std::string, Special>;

class Lexer final: tfl::Regexes<char> {
    using Regex = tfl::Regex<char>;

    Regex const num     = literal([](char c){ return '0' <= c && c <= '9'; });
    Regex const alpha   = range('a', 'z') | Regexes::range('A', 'Z');
    Regex const plus    = literal('+');
    Regex const minus   = literal('-');
    Regex const time    = literal('*');
    Regex const space   = any_literal({'\t', '\n', '\v', '\f', '\r', ' '});

    tfl::Lexer<char, Token, std::string> const lexer = tfl::Lexer<char, Token, std::string>::make({
        {opt(minus) & *num, [](auto w){ return Token{std::stoi(w)}; }},
        {*alpha, [](auto w){ return Token{w}; }},
        {*space, [](auto){ return Token{Special::SPACE}; }},
        {plus, [](auto){ return Token{Special::PLUS}; }},
        {time, [](auto){ return Token{Special::TIME}; }},
    }).map([](auto v){ return v.value(); })
        .filter([](auto v){ return v != Token{Special::SPACE}; });

public:
    std::vector<Token> operator()(std::string const& input) const {
        return lexer(input.cbegin(), input.cend());
    }
};


class Parser final: tfl::Parsers<Token> {
    template<typename R>
    using Syntax = tfl::Parser<Token, R>;

    template<typename R>
    Syntax<R> token() {
        return elem([](auto v){ return std::holds_alternative<R>(v); })
            .map([](auto v){ return std::get<R>(v); });
    }

    Syntax<int> build_parser() {
        Syntax<int> literal = token<int>();
        Syntax<Token> plus = elem(Token{Special::PLUS});
        Syntax<Token> time = elem(Token{Special::TIME});

        Syntax<int> prod = repsep1(literal, time).map([](auto ls){ return std::accumulate(ls.cbegin(), ls.cend(), 1, [](auto l, auto r){ return l*r; }); });
        Syntax<int> sum = repsep1(prod, plus).map([](auto ls){ return std::accumulate(ls.cbegin(), ls.cend(), 0); });

        return sum;
    }

    Syntax<int> parser = build_parser();

public:
    auto operator()(std::vector<Token> const& input) const {
        return parser(input.cbegin(), input.cend());
    }
};


int main(int argc, char** argv) {
    Lexer lexer;
    Parser parser;

    std::string input("-3 + 4 + 5*2");
    auto tokens = lexer(input);

    std::cout << parser(tokens) << std::endl;

    return 0;
}