#include "Lexer.hpp"

#include <tfl/Lexer.hpp>

static std::vector<char> operator ""_v(char const* str, std::size_t s) {
    return std::vector(str, str+s);
}

class Lexer final: tfl::Regexes<char> {
    using Regex = tfl::Regex<char>;

    Regex const alpha       = literal([](auto chr){ 
        return ('a' <= chr && chr <= 'z') || ('A' <= chr && chr <= 'Z');
    });
    Regex const digit       = literal([](auto chr){
        return ('0' <= chr && chr <= '9');
    });

    Regex const keyword     = word("def"_v) | word("extern"_v) | word("if"_v) | word("then"_v) | word("else"_v);
    Regex const space       = any_of("\t\n\v\f\r "_v);
    Regex const identifier  = alpha & *(alpha | digit);
    Regex const number      = +(digit | literal('.'));

    Regex const sharp       = literal('#');
    Regex const no_nl       = literal([](auto chr){ return chr != '\n'; });

    tfl::Lexer<char, Token, std::string> const lexer = tfl::Lexer<char, Token, std::string>::make({
        {keyword, [](auto w){ return Token{from_string(w)}; }},
        {identifier, [](auto w){ return Token{w}; }},
        {space, [](auto){ return Token{Special::SPACE}; }},
        {number, [](auto w){ return std::stod(w); }},
        {sharp & *no_nl, [](auto){ return Token{Special::COMMENT}; }},
        {any_literal(), [](auto w){ return Token{w[0]}; }}
    }).map([](auto v){ return v.value(); })
        .filter([](auto v){ return (v != Token{Special::SPACE}) && (v != Token{Special::COMMENT}); });

public:
    std::vector<Token> operator()(std::string const& input) const {
        return lexer(input.cbegin(), input.cend());
    }
};

std::vector<Token> lex(std::string const& input) {
    static Lexer lexer;
    return lexer(input);
}