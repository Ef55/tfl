#include <catch2/catch.hpp>

#include "tfl/Parser.hpp"

template<typename R>
using Parser = tfl::Parser<char, R>;

TEST_CASE("Parser input tests") {

    SECTION("Elem") {
        Parser<char> p(Parser<char>::elem([](char c){ return c == 'a'; }));

        REQUIRE( p({}) == std::vector<char>{} );
        REQUIRE( p({'a'}) == std::vector<char>{'a'} );
        REQUIRE( p({'b'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'a'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Epsilon") {
        Parser<char> p(Parser<char>::eps([](){ return 'a'; }));

        REQUIRE( p({}) == std::vector<char>{'a'} );
        REQUIRE( p({'a'}) == std::vector<char>{} );
        REQUIRE( p({'b'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'a'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Disjunction") {
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<char> p2(Parser<char>::eps([](){ return 'b'; }));
        Parser<char> p = p1 | p2;


        REQUIRE( p({}) == std::vector<char>{'b'} );
        REQUIRE( p({'a'}) == std::vector<char>{'a'} );
        REQUIRE( p({'b'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'a'}) == std::vector<char>{} );
        REQUIRE( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Sequence") {
        using R = std::pair<char, int>;
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<int> p2(Parser<int>::eps([](){ return 1; }));
        Parser<R> p = p1 & p2;

        REQUIRE( p({}) == std::vector<R>{} );
        REQUIRE( p({'a'}) == std::vector<R>{{'a', 1}} );
        REQUIRE( p({'b'}) == std::vector<R>{} );
        REQUIRE( p({'a', 'a'}) == std::vector<R>{} );
        REQUIRE( p({'a', 'b'}) == std::vector<R>{} );
    }

    SECTION("Map") {
        Parser<char> p1(Parser<char>::elem([](char c){ return true; }));
        Parser<int> p = p1.map([](char i)->int{ return i*i; });

        REQUIRE( p({}) == std::vector<int>{} );
        REQUIRE( p({3}) == std::vector<int>{9} );
        REQUIRE( p({8}) == std::vector<int>{64} );
        REQUIRE( p({1, 1}) == std::vector<int>{} );
    }

}