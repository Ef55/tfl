#include <catch2/catch_test_macros.hpp>

#include "tfl/Parser.hpp"

#include <functional>
#include <type_traits>

template<typename R>
using Parser = tfl::Parser<char, R>;

template<typename R>
using Recursive = tfl::Recursive<char, R>;

TEST_CASE("Parser input tests") {

    SECTION("Elem") {
        Parser<char> p(Parser<char>::elem([](char c){ return c == 'a'; }));

        CHECK_THROWS( p({}) );
        CHECK( p({'a'}) == 'a' );
        CHECK_THROWS( p({'b'}) );
        CHECK_THROWS( p({'a', 'a'}) );
        CHECK_THROWS( p({'a', 'b'}) );
    }

    SECTION("Epsilon") {
        Parser<char> p(Parser<char>::eps('a'));

        CHECK( p({}) == 'a' );
        CHECK_THROWS( p({'a'}) );
        CHECK_THROWS( p({'b'}) );
        CHECK_THROWS( p({'a', 'a'}) );
        CHECK_THROWS( p({'a', 'b'}) );
    }

    SECTION("Disjunction") {
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<char> p2(Parser<char>::eps('b'));
        Parser<char> p = p1 | p2;


        CHECK( p({}) == 'b' );
        CHECK( p({'a'}) == 'a' );
        CHECK_THROWS( p({'b'}) );
        CHECK_THROWS( p({'a', 'a'}) );
        CHECK_THROWS( p({'a', 'b'}) );
    }

    SECTION("Sequence") {
        using R = std::pair<char, int>;
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<int> p2(Parser<int>::eps(1));
        Parser<R> p = p1 & p2;

        CHECK_THROWS( p({}) );
        CHECK( p({'a'}) == R{'a', 1} );
        CHECK_THROWS( p({'b'}) );
        CHECK_THROWS( p({'a', 'a'}) );
        CHECK_THROWS( p({'a', 'b'}) );
    }

    SECTION("Map") {
        Parser<char> p1(Parser<char>::elem([](char c){ return true; }));
        Parser<int> p = p1.map([](char i)->int{ return i*i; });

        CHECK_THROWS( p({}) );
        CHECK( p({3}) == 9 );
        CHECK( p({8}) == 64 );
        CHECK_THROWS( p({1, 1}) );
    }

    SECTION("Recursion") {
        Recursive<int> rec;
        Parser<int> p = rec = 
            Parser<int>::eps(0) | 
            (Parser<char>::elem([](char c){ return true; }) & rec)
                .map([](std::pair<char, char> pair)->int{ return pair.first + pair.second; });

        CHECK( p({}) == 0 );
        CHECK( p({1}) == 1 );
        CHECK( p({1, 10}) == 11 );
        CHECK( p({1, 10, 100}) == 111 );
    }
}

TEST_CASE("Recursion operators") {

    SECTION("Disjunction") {
        Parser<char> pc = Parser<char>::eps('a');

        Recursive<char> rec1;
        Recursive<char> rec2;
        auto p1 = rec1 = pc | rec1;
        auto p2 = rec2 = rec2 | pc;

        STATIC_REQUIRE( std::is_same_v<decltype(p1), Parser<char>> );
        STATIC_REQUIRE( std::is_same_v<decltype(p2), Parser<char>> );
    }

    SECTION("Sequence") {
        auto sum = [](auto p){ return p.first+p.second; };
        Parser<int> pi = Parser<int>::eps(0);

        Recursive<int> rec1;
        Recursive<int> rec2;
        auto p1 = rec1 = (pi & rec1).map(sum);
        auto p2 = rec2 = (rec2 & pi).map(sum);

        STATIC_REQUIRE( std::is_same_v<decltype(p1), Parser<int>> );
        STATIC_REQUIRE( std::is_same_v<decltype(p2), Parser<int>> );
    }

    SECTION("Map") {
        Recursive<int> rec;
        auto p = rec = rec.map([](auto i){ return i+1; });

        STATIC_REQUIRE( std::is_same_v<decltype(p), Parser<int>> );
    }
}

TEST_CASE("Cross-recursion") {
    Recursive<int> rec1;
    Recursive<int> rec2;

    rec1 = Parser<int>::eps(0) | 
        (Parser<char>::elem([](char c){ return c=='a'; }) & (rec1 | rec2))
            .map([](auto p){ return p.second + 1; });

    rec2 =  
        (Parser<char>::elem([](char c){ return c=='b'; }) & (rec1 | rec2))
            .map([](auto p){ return p.second + 2; });
    
    Parser<int> p = rec1 | rec2;

    CHECK( p({}) == 0 );
    CHECK( p({'a'}) == 1 );
    CHECK( p({'b'}) == 2 );
    CHECK( p({'a', 'a', 'a', 'b', 'b', 'a'}) == 8 );
}