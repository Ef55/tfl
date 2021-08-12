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
        auto elem = Parsers::elem([](auto){ return true; });
        Parser<std::vector<char>> p = Parsers::many(elem);

        CHECK( p({}) == std::vector<char>{} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'a', 'b'}) == std::vector<char>{'a', 'b'} );
        CHECK( p({'a', 'b', 'c'}) == std::vector<char>{'a', 'b', 'c'} );
    }
}