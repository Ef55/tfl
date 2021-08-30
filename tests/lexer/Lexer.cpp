#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

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
        auto result = lexer(input);

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
        auto result = lexer(input);

        CHECK( result.size() == expected.size() );

        for(size_t i(0); i < expected.size(); ++i) {
            INFO( std::to_string(i) + "th word" );
            CHECK( result[i] == expected[i] );
        }
    }
}

struct DerivationLexer {
    template<typename T, typename R, class Word = std::vector<T>>
    static tfl::Lexer<T, tfl::Positioned<R>, Word> make(std::initializer_list<tfl::Rule<tfl::Regex<T>, R, Word>> rules, tfl::Regex<T> newline = tfl::Regex<T>::empty()) {
        return tfl::Lexer<T, R, Word>::make_derivation_lexer(rules, newline);
    }
};

struct DFALexer {
    template<typename T, typename R, class Word = std::vector<T>>
    static tfl::Lexer<T, tfl::Positioned<R>, Word> make(std::initializer_list<tfl::Rule<tfl::Regex<T>, R, Word>> rules, tfl::Regex<T> newline = tfl::Regex<T>::empty()) {
        return tfl::Lexer<T, R, Word>::make_dfa_lexer(rules, newline);
    }
};

#define LEXERS DerivationLexer, DFALexer

TEMPLATE_TEST_CASE("Simple usecase", "[template]", LEXERS) {
    using R = std::variant<std::string, int, SpecialSymbol>;
    using Regexes = tfl::Regexes<char>;

    auto alpha = Regexes::range('a', 'z') | Regexes::range('A', 'Z');

    auto digit = Regexes::range('0', '9');

    auto eol = Regexes::literal('\n');

    auto space = Regexes::any_of({'\t', '\n', '\v', '\f', '\r', ' '});


    auto lexer = TestType::template make<char, R>({
        {Regexes::any_of({
            Regexes::word({'i', 'f'}),
            Regexes::word({'t', 'h', 'e', 'n'}),
            Regexes::word({'e', 'l', 's', 'e'}),
            Regexes::word({'r', 'e', 't', 'u', 'r', 'n'})
            }), [](auto w){ return R(SpecialSymbol::KEYWORD); }},
        {*alpha, [](auto w){ return R(std::string(w.begin(), w.end())); }},
        {*digit, [](auto w){ return R(std::stoi(std::string(w.begin(), w.end()))); }},
        {Regexes::literal('('), [](auto w){ return R(SpecialSymbol::OP_PAR); }},
        {Regexes::literal(')'), [](auto w){ return R(SpecialSymbol::CL_PAR); }},
        {*space, [](auto w){ return R(SpecialSymbol::SEP); }},
        {Regexes::literal('+') | Regexes::literal('-') | Regexes::literal('/') | Regexes::literal('*'), [](auto w){ return R(SpecialSymbol::OP); }},
        {Regexes::literal('/') - Regexes::literal('/') - *(digit | alpha | Regexes::literal(' ')) - eol, [](auto w){ return R(SpecialSymbol::COMMENT); }},
    });

    auto integer_lexer = TestType::template make<char, int, std::string>({
        {*digit, [](auto str){ return std::stoi(str); }}
    }).map([](auto e){ return e.value(); });

    test_lexer_line_positioned(
        lexer,
        "Simple arithmetic expression is lexed as expected",
        "12x+4",
        {
            {1, R(12)},
            {3, R(std::string("x"))},
            {4, R(SpecialSymbol::OP)},
            {5, R(4)}
        }
    );

    test_lexer_line_positioned(
        lexer,
        "Maximal munch is used", 
        "//th15 15 a c0mment\n",
        {
            {1, R(SpecialSymbol::COMMENT)}
        }
    );

    test_lexer_line_positioned(
        lexer,
        "Priority is used", 
        "if",
        {
            {1, R(SpecialSymbol::KEYWORD)}
        }
    );

    test_lexer_line_positioned(
        lexer,
        "Monoline expression is lexed as expected",
        "return if (x equals 12) then (3) else (potato)",
        {
            {1, R(SpecialSymbol::KEYWORD)},
            {7, R(SpecialSymbol::SEP)},
            {8, R(SpecialSymbol::KEYWORD)},
            {10, R(SpecialSymbol::SEP)},
            {11, R(SpecialSymbol::OP_PAR)},
            {12, R(std::string("x"))},
            {13, R(SpecialSymbol::SEP)},
            {14, R(std::string("equals"))},
            {20, R(SpecialSymbol::SEP)},
            {21, R(12)},
            {23, R(SpecialSymbol::CL_PAR)},
            {24, R(SpecialSymbol::SEP)},
            {25, R(SpecialSymbol::KEYWORD)},
            {29, R(SpecialSymbol::SEP)},
            {30, R(SpecialSymbol::OP_PAR)},
            {31, R(3)},
            {32, R(SpecialSymbol::CL_PAR)},
            {33, R(SpecialSymbol::SEP)},
            {34, R(SpecialSymbol::KEYWORD)},
            {38, R(SpecialSymbol::SEP)},
            {39, R(SpecialSymbol::OP_PAR)},
            {40, R(std::string("potato"))},
            {46, R(SpecialSymbol::CL_PAR)},
        }
    );
    
    test_lexer(
        lexer.map([](auto p){ return p.value(); }),
        "Lexer map can be used to drop position",
        "12x+4",
        {
            R(12),
            R(std::string("x")),
            R(SpecialSymbol::OP),
            R(4)
        }
    );
}