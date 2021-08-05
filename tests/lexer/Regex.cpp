#include <catch2/catch.hpp>

#include "tfl/Regex.hpp"

TEMPLATE_TEST_CASE("Regex input tests", "[template]", tfl::Regex<char>) {

    SECTION("Empty") {
        TestType r = TestType::empty();

        REQUIRE( !r.accepts({}) );
        REQUIRE( !r.accepts({'a'}) );
        REQUIRE( !r.accepts({'b'}) );
        REQUIRE( !r.accepts({'a', 'b'}) );
    }

    SECTION("Epsilon") {
        TestType r = TestType::epsilon();

        REQUIRE( r.accepts({}) );
        REQUIRE( !r.accepts({'a'}) );
        REQUIRE( !r.accepts({'b'}) );
        REQUIRE( !r.accepts({'a', 'b'}) );
    }

    SECTION("Literal") {

        SECTION("'a'") {
            TestType a = TestType::literal('a');

            REQUIRE( !a.accepts({}) );
            REQUIRE( a.accepts({'a'}) );
            REQUIRE( !a.accepts({'b'}) );
            REQUIRE( !a.accepts({'a', 'b'}) );
        }

        SECTION("'b'") {
            TestType b = TestType::literal('b');

            REQUIRE( !b.accepts({}) );
            REQUIRE( !b.accepts({'a'}) );
            REQUIRE( b.accepts({'b'}) );
            REQUIRE( !b.accepts({'a', 'b'}) );
        }
    }

    SECTION("Disjunction") {
        TestType a = TestType::literal('a');
        TestType b = TestType::literal('b');
        TestType e = TestType::epsilon();

        TestType ab = a | b;
        TestType r = ab | e;

        REQUIRE( !ab.accepts({}) );
        REQUIRE( ab.accepts({'a'}) );
        REQUIRE( ab.accepts({'b'}) );
        REQUIRE( !ab.accepts({'a', 'b'}) );


        REQUIRE( r.accepts({}) );
        REQUIRE( r.accepts({'a'}) );
        REQUIRE( r.accepts({'b'}) );
        REQUIRE( !r.accepts({'a', 'b'}) );
    }

    SECTION("Sequence") {
        TestType a = TestType::literal('a');
        TestType b = TestType::literal('b');
        TestType e = TestType::epsilon();

        TestType ab = a & b;
        TestType abe = ab & e;
        TestType aba = ab & a;

        REQUIRE( !ab.accepts({}) );
        REQUIRE( !ab.accepts({'a'}) );
        REQUIRE( !ab.accepts({'b'}) );
        REQUIRE( ab.accepts({'a', 'b'}) );
        REQUIRE( !ab.accepts({'a', 'b', 'a'}) );

        REQUIRE( !abe.accepts({}) );
        REQUIRE( !abe.accepts({'a'}) );
        REQUIRE( !abe.accepts({'b'}) );
        REQUIRE( abe.accepts({'a', 'b'}) );
        REQUIRE( !abe.accepts({'a', 'b', 'a'}) );

        REQUIRE( !aba.accepts({}) );
        REQUIRE( !aba.accepts({'a'}) );
        REQUIRE( !aba.accepts({'b'}) );
        REQUIRE( !aba.accepts({'a', 'b'}) );
        REQUIRE( aba.accepts({'a', 'b', 'a'}) );
    }

    SECTION("Closure") {
        TestType a = TestType::literal('a');
        TestType b = TestType::literal('b');
        TestType c = TestType::literal('c');

        TestType r = *((a & b) | c);

        REQUIRE( r.accepts({}) );
        REQUIRE( !r.accepts({'a'}) );
        REQUIRE( !r.accepts({'b'}) );
        REQUIRE( r.accepts({'c'}) );
        REQUIRE( r.accepts({'a', 'b'}) );
        REQUIRE( r.accepts({'a', 'b', 'c'}) );
        REQUIRE( r.accepts({'a', 'b', 'a', 'b'}) );
        REQUIRE( r.accepts({'c', 'a', 'b', 'a', 'b', 'c'}) );
    }

}

TEMPLATE_TEST_CASE("Regex utilities input tests", "[template]", tfl::Regexes<char>) {

    SECTION("Word") {
        auto r = TestType::word({'t', 'o', 'm', 'a', 't', 'o'});

        REQUIRE( !r.accepts({}) );
        REQUIRE( r.accepts({'t', 'o', 'm', 'a', 't', 'o'}) );
        REQUIRE( !r.accepts({'o', 'm', 'a', 't', 'o'}) );
        REQUIRE( !r.accepts({'t', 'o', 'm', 'a', 't'}) );
        REQUIRE( !r.accepts({'t', 'o', 'm', 'a', 't', 'o', 'e', 's'}) );
        REQUIRE( !r.accepts({'t', 'o', 'm', 'e', 't', 'o'}) );
    }

    SECTION("Any (literal)") {
        auto r = TestType::any_literal({'t', 'o', 'm', 'a'});

        REQUIRE( !r.accepts({}) );
        REQUIRE( r.accepts({'t'}) );
        REQUIRE( r.accepts({'o'}) );
        REQUIRE( r.accepts({'m'}) );
        REQUIRE( r.accepts({'a'}) );
        REQUIRE( !r.accepts({'t', 'o', 'm', 'a'}) );
        REQUIRE( !r.accepts({'t', 'o', 'm', 'a', 't', 'o'}) );
    }

    SECTION("Range") {
        auto r = TestType::range('2', '4');

        REQUIRE( !r.accepts({'0'}) );
        REQUIRE( !r.accepts({'1'}) );
        REQUIRE( r.accepts({'2'}) );
        REQUIRE( r.accepts({'3'}) );
        REQUIRE( r.accepts({'4'}) );
        REQUIRE( !r.accepts({'5'}) );
        REQUIRE( !r.accepts({'6'}) );
        REQUIRE( !r.accepts({'7'}) );
        REQUIRE( !r.accepts({'8'}) );
        REQUIRE( !r.accepts({'9'}) );
    }
}