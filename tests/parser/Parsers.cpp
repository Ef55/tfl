#include <catch2/catch.hpp>

#include "tfl/Parser.hpp"

#include <functional>
#include <type_traits>
#include <iostream>

template<typename R>
using Parser = tfl::Parser<char, R>;
using Parsers = tfl::Parsers<char>;

TEST_CASE("Additional combinators") {
    SECTION("Opt") {
        auto p = Parsers::opt(Parsers::any());

        CHECK( p({}) == std::nullopt );
        CHECK( p({'a'}) == std::optional('a') );
        CHECK( p({'b'}) == std::optional('b') );
        CHECK_THROWS( p({'a', 'a'}) );
        CHECK_THROWS( p({'a', 'b'}) );
    }

    SECTION("Many") {
        auto elem = Parsers::any();
        Parser<std::vector<char>> p = Parsers::many(elem);

        CHECK( p({}) == std::vector<char>{} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', 'b', 'c'}) == std::vector<char>{'a', 'b', 'c'} );
    }

    SECTION("Many 1") {
        auto elem = Parsers::any();
        Parser<std::vector<char>> p = Parsers::many1(elem);

        CHECK_THROWS( p({}) );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', 'b', 'c'}) == std::vector<char>{'a', 'b', 'c'} );
    }

    SECTION("Rep Sep") {
        auto elem = Parsers::elem([](auto c){ return 'a' <= c && c<= 'z'; });
        auto sep = Parsers::elem([](auto c){ return c == ','; });
        Parser<std::vector<char>> p = Parsers::repsep(elem, sep);

        CHECK( p({}) == std::vector<char>{} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', ',', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', ',', 'b', ',', 'c'}) == std::vector<char>{'a', 'b', 'c'} );

        CHECK_THROWS( p({','}) );
        CHECK_THROWS( p({',', ','}) );
        CHECK_THROWS( p({'a', ','}) );
        CHECK_THROWS( p({'a', ',', 'b', ','}) );
        CHECK_THROWS( p({'a', ',', ',', 'b', ',', 'c'}) );
    }

    SECTION("Rep Sep 1") {
        auto elem = Parsers::elem([](auto c){ return 'a' <= c && c<= 'z'; });
        auto sep = Parsers::elem([](auto c){ return c == ','; });
        Parser<std::vector<char>> p = Parsers::repsep1(elem, sep);

        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', ',', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', ',', 'b', ',', 'c'}) == std::vector<char>{'a', 'b', 'c'} );

        CHECK_THROWS( p({}) );
        CHECK_THROWS( p({','}) );
        CHECK_THROWS( p({',', ','}) );
        CHECK_THROWS( p({'a', ','}) );
        CHECK_THROWS( p({'a', ',', 'b', ','}) );
        CHECK_THROWS( p({'a', ',', ',', 'b', ',', 'c'}) );
    }

    SECTION("Either") {
        using R = std::variant<char, int, std::string>;
        Parser<char> alpha = Parsers::elem([](auto c){ return 'a' <= c && c <= 'z'; });
        Parser<int> num = Parsers::elem([](auto c){ return '0' <= c && c <= '9'; }).map([](char c)->int{ return c - '0'; });
        Parser<std::string> hex = Parsers::elem([](auto c){ return c < 10; }).map([](char c)->std::string{ return "0x" + std::to_string(c); });

        auto p = Parsers::either(alpha, num, hex);

        CHECK( p({'a'}) == R{'a'} );
        CHECK( p({'z'}) == R{'z'} );
        CHECK( p({'0'}) == R{0} );
        CHECK( p({'9'}) == R{9} );
        CHECK( p({'\0'}) == R{"0x0"} );
        CHECK( p({'\7'}) == R{"0x7"} );
    }
}