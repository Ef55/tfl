#include "Lexer.hpp"

#include <tfl/Lexer.hpp>

static std::vector<char> operator ""_v(char const* str, std::size_t s) {
    return std::vector(str, str+s);
}

static class: tfl::Regexes<char> {
    using Regex = tfl::Regex<char>;

    // Miscellaneous
    Regex const whitespace = any_of(" \n\r\t"_v);
    Regex const special = any_of("{}[],:"_v);

    // String
    Regex const quote = literal('"');
    Regex const str_char = literal([](char c){ return c != '"' && c != '\\' && (unsigned char)c >= u' '; });
    Regex const ctr_char = any_of("\"\\/bfnrt"_v);
    Regex const hex_digit = range('0', '9') | range('a', 'f') | range('A', 'F');
    Regex const unicode = literal('u') & hex_digit & hex_digit & hex_digit & hex_digit;
    Regex const control = literal('\\') & (ctr_char | unicode);
    Regex const string = quote & *(str_char | control) & quote;

    // Boolean
    Regex const truu = word("true"_v);
    Regex const falz = word("false"_v);

    // Null
    Regex const null = word("null"_v);

    // Number
    Regex const digit19 = range('1', '9');
    Regex const digit = range('0', '9');

    Regex const number_base = opt(literal('-')) & (literal('0') | (digit19 & *digit));
    Regex const number_fraction = literal('.') & +digit;
    Regex const number_exponent = any_of("eE"_v) & opt(any_of("+-"_v)) & +digit;

    Regex const number = number_base & opt(number_fraction) & opt(number_exponent);

    tfl::Lexer<char, Token, std::string> const lexer = tfl::Lexer<char, Token, std::string>::make({
        {+whitespace,    [](auto){   return ' '; }},
        {special,       [](auto w){ return w[0]; }},
        {string,        [](auto s){ return s.substr(1, s.size()-2); }},
        {truu,          [](auto){   return true; }},
        {falz,          [](auto){   return false; }},
        {null,          [](auto){   return nullptr; }},
        {number,        [](auto w){ return std::stod(w); }},
    }).map([](auto v){ return v.value(); });

public:
    std::vector<Token> operator()(std::string const& input) const {
        return lexer(input.cbegin(), input.cend());
    }
} lexer;

std::vector<Token> lex(std::string const& input) {
    return lexer(input);
}