#include <catch2/catch.hpp>

#include "tfl/Lexer.hpp"

#include <variant>

namespace Catch {
    template<typename T>
    struct StringMaker<tfl::Positioned<T>> {
        static std::string convert( tfl::Positioned<T> const& value ) {
            return StringMaker<T>::convert(value.value()) + " @"  + std::to_string(value.line()) + ";" + std::to_string(value.column());
        }
    };

    template<typename... Args>
    struct StringMaker<std::variant<Args...>> {

        static std::string convert( std::variant<Args...> const& value ) {
            return std::visit([](auto&& t){ return "variant{" + StringMaker<decltype(t)>::convert(t) + "}"; }, value);
            //return std::visit(visitor, value);
        }
    };
}

enum class SpecialSymbol {
    OP_PAR,
    CL_PAR,
    SEP,
    OP,
    COMMENT,
    KEYWORD
};
CATCH_REGISTER_ENUM( SpecialSymbol, SpecialSymbol::OP_PAR, SpecialSymbol::CL_PAR, SpecialSymbol::SEP, SpecialSymbol::OP, SpecialSymbol::COMMENT )

template<typename R>
void test_lexer(tfl::Lexer<char, tfl::Positioned<R>> lexer, char const* name, char const* cinput, std::vector<std::pair<size_t, R>> expected) {
    SECTION(name) {

        std::string input(cinput);
        auto result = lexer(input.begin(), input.end());

        CHECK( result.size() == expected.size() );

        for(size_t i(0); i < expected.size(); ++i) {
            INFO( std::to_string(i) + "th word" );
            REQUIRE( result[i] == tfl::Positioned<R>(1, expected[i].first, expected[i].second));
        }
    }
}

TEST_CASE("Simple usecase") {
    using Word = std::variant<std::string, int, SpecialSymbol>;
    using Regex = tfl::Regex<char>;
    using Regexes = tfl::Regexes<char>;

    auto alpha = Regexes::range('a', 'z') | Regexes::range('A', 'Z');

    auto digit = Regex::literal([](auto chr){ return ('0' <= chr && chr <= '9'); });

    auto eol = Regex::literal('\n');

    auto space = Regexes::any_literal({'\t', '\n', '\v', '\f', '\r', ' '});


    auto lexer = tfl::Lexer<char, Word>::make({
        {Regexes::any({
            Regexes::word({'i', 'f'}),
            Regexes::word({'t', 'h', 'e', 'n'}),
            Regexes::word({'e', 'l', 's', 'e'}),
            Regexes::word({'r', 'e', 't', 'u', 'r', 'n'})
            }), [](auto w){ return Word(SpecialSymbol::KEYWORD); }},
        {*alpha, [](auto w){ return Word(std::string(w.begin(), w.end())); }},
        {*digit, [](auto w){ return Word(std::stoi(std::string(w.begin(), w.end()))); }},
        {Regex::literal('('), [](auto w){ return Word(SpecialSymbol::OP_PAR); }},
        {Regex::literal(')'), [](auto w){ return Word(SpecialSymbol::CL_PAR); }},
        {*space, [](auto w){ return Word(SpecialSymbol::SEP); }},
        {Regex::literal('+') | Regex::literal('-') | Regex::literal('/') | Regex::literal('*'), [](auto w){ return Word(SpecialSymbol::OP); }},
        {Regex::literal('/') & Regex::literal('/') & *(digit | alpha | Regex::literal(' ')) & eol, [](auto w){ return Word(SpecialSymbol::COMMENT); }},
    });

    test_lexer(
        lexer,
        "Simple arithmetic expression is lexed as expected",
        "12x+4",
        {
            {1, Word(12)},
            {3, Word(std::string("x"))},
            {4, Word(SpecialSymbol::OP)},
            {5, Word(4)}
        }
    );

    test_lexer(
        lexer,
        "Maximal munch is used", 
        "//th15 15 a c0mment\n",
        {
            {1, Word(SpecialSymbol::COMMENT)}
        }
    );

    test_lexer(
        lexer,
        "Priority is used", 
        "if",
        {
            {1, Word(SpecialSymbol::KEYWORD)}
        }
    );

    test_lexer(
        lexer,
        "Monoline expression is lexed as expected",
        "return if (x equals 12) then (3) else (potato)",
        {
            {1, Word(SpecialSymbol::KEYWORD)},
            {7, Word(SpecialSymbol::SEP)},
            {8, Word(SpecialSymbol::KEYWORD)},
            {10, Word(SpecialSymbol::SEP)},
            {11, Word(SpecialSymbol::OP_PAR)},
            {12, Word(std::string("x"))},
            {13, Word(SpecialSymbol::SEP)},
            {14, Word(std::string("equals"))},
            {20, Word(SpecialSymbol::SEP)},
            {21, Word(12)},
            {23, Word(SpecialSymbol::CL_PAR)},
            {24, Word(SpecialSymbol::SEP)},
            {25, Word(SpecialSymbol::KEYWORD)},
            {29, Word(SpecialSymbol::SEP)},
            {30, Word(SpecialSymbol::OP_PAR)},
            {31, Word(3)},
            {32, Word(SpecialSymbol::CL_PAR)},
            {33, Word(SpecialSymbol::SEP)},
            {34, Word(SpecialSymbol::KEYWORD)},
            {38, Word(SpecialSymbol::SEP)},
            {39, Word(SpecialSymbol::OP_PAR)},
            {40, Word(std::string("potato"))},
            {46, Word(SpecialSymbol::CL_PAR)},
        }
    );
    
}