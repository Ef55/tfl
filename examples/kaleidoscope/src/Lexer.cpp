#include "Lexer.hpp"

#include <tfl/Lexer.hpp>

static std::vector<char> operator ""_v(char const* str, std::size_t s) {
    return std::vector(str, str+s);
}

static class: tfl::Regexes<char> {
    using Regex = tfl::Regex<char>;

    Regex const alpha = range('a', 'z') | range('A', 'Z');
    Regex const digit = range('0', '9');

    Regex const keyword     = any_of({word("def"_v), word("extern"_v), word("if"_v), word("then"_v), word("else"_v)});
    Regex const space       = any_of("\t\n\v\f\r "_v);
    Regex const identifier  = alpha - *(alpha | digit);
    Regex const number      = +(digit | literal('.'));

    Regex const sharp       = literal('#');
    Regex const no_nl       = alphabet() / literal('\n');

    tfl::Lexer<char, Token, std::string> const lexer = tfl::Lexer<char, Token, std::string>::make({
        {keyword,           [](auto w){ return from_string(w); }},
        {identifier,        [](auto w){ return w; }},
        {space,             [](auto){   return Special::SPACE; }},
        {number,            [](auto w){ return std::stod(w); }},
        {sharp - *no_nl,    [](auto){   return Special::COMMENT; }},
        {alphabet(),        [](auto w){ return w[0]; }}
    }).map([](auto v){ return v.value(); })
      .filter([](auto v){ return (v != Token{Special::SPACE}) && (v != Token{Special::COMMENT}); });

public:
    std::vector<Token> operator()(std::string const& input) const {
        return lexer(input.cbegin(), input.cend());
    }
} lexer;

std::vector<Token> lex(std::string const& input) {
    return lexer(input);
}