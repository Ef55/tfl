#include <catch2/catch.hpp>

#include "tfl/Parser.hpp"

#include <functional>
#include <type_traits>
#include <iostream>

template<typename R>
using Parser = tfl::Parser<char, R>;
using Parsers = tfl::Parsers<char>;

TEST_CASE("Additional combinators") {
    SECTION("Many") {
        auto elem = Parsers::success();
        Parser<std::vector<char>> p = Parsers::many(elem);

        CHECK( p({}) == std::vector<char>{} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', 'b', 'c'}) == std::vector<char>{'a', 'b', 'c'} );
    }

    SECTION("Many1") {
        auto elem = Parsers::success();
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
}