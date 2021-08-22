#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regexes = tfl::Regexes<char>;
using Size = tfl::RegexesMetrics<char>::Size;

static auto to_string = tfl::RegexesPrinter<char>::to_string;

TEST_CASE("Regexes are printed as expected") {
    auto a = Regexes::literal('a');
    auto f = Regexes::empty();
    auto e = Regexes::epsilon();

    SECTION("Base cases") {
        CHECK( to_string(a) == "a" );
        CHECK( to_string(f) == "∅" );
        CHECK( to_string(e) == "ε" );
        CHECK( to_string(a & a) == "aa" );
        CHECK( to_string(a | a) == "a | a" );
        CHECK( to_string(*a) == "*a" );
    }

    SECTION("Sequence associativity") {
        CHECK( to_string(a & a & a) == "aaa" );
        CHECK( to_string(a & (a & a)) == "a(aa)" );
    }

    SECTION("Disjunction associativity") {
        CHECK( to_string(a | a | a) == "a | a | a" );
        CHECK( to_string(a | (a | a)) == "a | (a | a)" );
    }

    SECTION("Sequence/disjunction combinations") {
        CHECK( to_string((a & a) | (a & a)) == "aa | aa" );
        CHECK( to_string(a & (a | a) & a) == "a(a | a)a" );
    }

    SECTION("Closure/binary combinations") {
        CHECK( to_string((*a & a) | a) == "*aa | a" );
        CHECK( to_string(*(a & a) | a) == "*(aa) | a" );
        CHECK( to_string(*((a & a) | a)) == "*(aa | a)" );
    }
}