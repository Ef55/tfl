#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regexes = tfl::Regexes<char>;
using Size = tfl::RegexesMetrics<char>::Size;

static auto to_string = tfl::RegexesPrinter<char>::to_string;

TEST_CASE("Regexes are printed as expected") {
    auto a = Regexes::literal('a');
    auto b = Regexes::literal('b');
    auto c = Regexes::literal('c');
    auto f = Regexes::empty();
    auto e = Regexes::epsilon();

    SECTION("Base cases") {
        CHECK( to_string(a) == "a" );
        CHECK( to_string(b) == "b" );
        CHECK( to_string(c) == "c" );
        CHECK( to_string(f) == "∅" );
        CHECK( to_string(e) == "ε" );
        CHECK( to_string(a & b) == "ab" );
        CHECK( to_string(a | b) == "a | b" );
        CHECK( to_string(*b) == "*b" );
        CHECK( to_string(-c) == "¬c" );
    }

    SECTION("Sequence associativity") {
        CHECK( to_string(a & b & c) == "abc" );
        CHECK( to_string(a & (b & c)) == "a(bc)" );
    }

    SECTION("Disjunction associativity") {
        CHECK( to_string(a | b | c) == "a | b | c" );
        CHECK( to_string(a | (b | c)) == "a | (b | c)" );
    }

    SECTION("Sequence/disjunction combinations") {
        CHECK( to_string((a & b) | (c & a)) == "ab | ca" );
        CHECK( to_string(a & (b | c) & a) == "a(b | c)a" );
    }

    SECTION("Closure/binary combinations") {
        CHECK( to_string((*a & b) | c) == "*ab | c" );
        CHECK( to_string(*(a & b) | c) == "*(ab) | c" );
        CHECK( to_string(*((a & b) | c)) == "*(ab | c)" );
    }

    SECTION("Complement/binary combinations") {
        CHECK( to_string((-a & b) | c) == "¬ab | c" );
        CHECK( to_string(-(a & b) | c) == "¬(ab) | c" );
        CHECK( to_string(-((a & b) | c)) == "¬(ab | c)" );
    }

    SECTION("Complement/closure combinations") {
        CHECK( to_string(-*a) == "¬*a" );
        CHECK( to_string(*-a) == "*¬a" );
        CHECK( to_string(*-*a) == "*¬*a" );
    }
}