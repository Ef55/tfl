#include <catch2/catch_test_macros.hpp>

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
void test_lexer_line_positioned(tfl::Lexer<char, tfl::Positioned<R>> lexer, char const* name, char const* cinput, std::vector<std::pair<size_t, R>> expected) {
    SECTION(name) {

        std::string input(cinput);
        auto result = lexer(input.begin(), input.end());

        CHECK( result.size() == expected.size() );

        for(size_t i(0); i < expected.size(); ++i) {
            INFO( std::to_string(i) + "th word" );
            CHECK( result[i] == tfl::Positioned<R>(1, expected[i].first, expected[i].second));
        }
    }
}

template<typename R>
void test_lexer(tfl::Lexer<char, R> lexer, char const* name, char const* cinput, std::vector<R> expected) {
    SECTION(name) {

        std::string input(cinput);
        auto result = lexer(input.begin(), input.end());

        CHECK( result.size() == expected.size() );

        for(size_t i(0); i < expected.size(); ++i) {
            INFO( std::to_string(i) + "th word" );
            CHECK( result[i] == expected[i] );
        }
    }
}

TEST_CASE("Simple usecase") {
    using Word = std::variant<std::string, int, SpecialSymbol>;
    using Regexes = tfl::Regexes<char>;

    auto alpha = Regexes::range('a', 'z') | Regexes::range('A', 'Z');

    auto digit = Regexes::range('0', '9');

    auto eol = Regexes::literal('\n');

    auto space = Regexes::any_of({'\t', '\n', '\v', '\f', '\r', ' '});


    auto lexer = tfl::Lexer<char, Word>::make({
        {Regexes::any_of({
            Regexes::word({'i', 'f'}),
            Regexes::word({'t', 'h', 'e', 'n'}),
            Regexes::word({'e', 'l', 's', 'e'}),
            Regexes::word({'r', 'e', 't', 'u', 'r', 'n'})
            }), [](auto w){ return Word(SpecialSymbol::KEYWORD); }},
        {*alpha, [](auto w){ return Word(std::string(w.begin(), w.end())); }},
        {*digit, [](auto w){ return Word(std::stoi(std::string(w.begin(), w.end()))); }},
        {Regexes::literal('('), [](auto w){ return Word(SpecialSymbol::OP_PAR); }},
        {Regexes::literal(')'), [](auto w){ return Word(SpecialSymbol::CL_PAR); }},
        {*space, [](auto w){ return Word(SpecialSymbol::SEP); }},
        {Regexes::literal('+') | Regexes::literal('-') | Regexes::literal('/') | Regexes::literal('*'), [](auto w){ return Word(SpecialSymbol::OP); }},
        {Regexes::literal('/') & Regexes::literal('/') & *(digit | alpha | Regexes::literal(' ')) & eol, [](auto w){ return Word(SpecialSymbol::COMMENT); }},
    });

    auto integer_lexer = tfl::Lexer<char, int, std::string>::make({
        {*digit, [](auto str){ return std::stoi(str); }}
    }).map([](auto e){ return e.value(); });

    test_lexer_line_positioned(
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

    test_lexer_line_positioned(
        lexer,
        "Maximal munch is used", 
        "//th15 15 a c0mment\n",
        {
            {1, Word(SpecialSymbol::COMMENT)}
        }
    );

    test_lexer_line_positioned(
        lexer,
        "Priority is used", 
        "if",
        {
            {1, Word(SpecialSymbol::KEYWORD)}
        }
    );

    test_lexer_line_positioned(
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
    
    test_lexer(
        lexer.map([](auto p){ return p.value(); }),
        "Lexer map can be used to drop position",
        "12x+4",
        {
            Word(12),
            Word(std::string("x")),
            Word(SpecialSymbol::OP),
            Word(4)
        }
    );
}