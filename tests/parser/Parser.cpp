#include <catch2/catch.hpp>

#include "tfl/Parser.hpp"
#include <functional>

template<typename R>
using Parser = tfl::Parser<char, R>;

TEST_CASE("Parser input tests") {

    SECTION("Elem") {
        Parser<char> p(Parser<char>::elem([](char c){ return c == 'a'; }));

        CHECK( p({}) == std::vector<char>{} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'b'}) == std::vector<char>{} );
        CHECK( p({'a', 'a'}) == std::vector<char>{} );
        CHECK( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Epsilon") {
        Parser<char> p(Parser<char>::eps([](){ return 'a'; }));

        CHECK( p({}) == std::vector<char>{'a'} );
        CHECK( p({'a'}) == std::vector<char>{} );
        CHECK( p({'b'}) == std::vector<char>{} );
        CHECK( p({'a', 'a'}) == std::vector<char>{} );
        CHECK( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Disjunction") {
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<char> p2(Parser<char>::eps([](){ return 'b'; }));
        Parser<char> p = p1 | p2;


        CHECK( p({}) == std::vector<char>{'b'} );
        CHECK( p({'a'}) == std::vector<char>{'a'} );
        CHECK( p({'b'}) == std::vector<char>{} );
        CHECK( p({'a', 'a'}) == std::vector<char>{} );
        CHECK( p({'a', 'b'}) == std::vector<char>{} );
    }

    SECTION("Sequence") {
        using R = std::pair<char, int>;
        Parser<char> p1(Parser<char>::elem([](char c){ return c == 'a'; }));
        Parser<int> p2(Parser<int>::eps([](){ return 1; }));
        Parser<R> p = p1 & p2;

        CHECK( p({}) == std::vector<R>{} );
        CHECK( p({'a'}) == std::vector<R>{{'a', 1}} );
        CHECK( p({'b'}) == std::vector<R>{} );
        CHECK( p({'a', 'a'}) == std::vector<R>{} );
        CHECK( p({'a', 'b'}) == std::vector<R>{} );
    }

    SECTION("Map") {
        Parser<char> p1(Parser<char>::elem([](char c){ return true; }));
        Parser<int> p = p1.map([](char i)->int{ return i*i; });

        CHECK( p({}) == std::vector<int>{} );
        CHECK( p({3}) == std::vector<int>{9} );
        CHECK( p({8}) == std::vector<int>{64} );
        CHECK( p({1, 1}) == std::vector<int>{} );
    }

    SECTION("Recursion") {
        std::function<Parser<int>()> parser = [&parser](){
            return Parser<int>::eps([](){ return 0; }) | 
            (Parser<char>::elem([](char c){ return true; }) & Parser<int>(parser))
                .map([](std::pair<char, char> pair)->int{ return pair.first + pair.second; });
        };
        Parser<int> p = parser;

        CHECK( p({}) == std::vector<int>{0} );
        CHECK( p({1}) == std::vector<int>{1} );
        CHECK( p({1, 10}) == std::vector<int>{11} );
        CHECK( p({1, 10, 100}) == std::vector<int>{111} );
    }
}
