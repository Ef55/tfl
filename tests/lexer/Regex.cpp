#include <catch2/catch.hpp>

#include "tfl/Regex.hpp"

TEMPLATE_TEST_CASE("Regex input tests", "[template]", tfl::RegexesDerivation<char>) {
    using Regex = tfl::Regex<char>;
    
    auto accepts = [](Regex r, std::initializer_list<char> ls){ return TestType::accepts(r, ls); };

    SECTION("Empty") {
        Regex r = Regex::empty();

        REQUIRE( !accepts(r, {}) );
        REQUIRE( !accepts(r, {'a'}) );
        REQUIRE( !accepts(r, {'b'}) );
        REQUIRE( !accepts(r, {'a', 'b'}) );
    }

    SECTION("Epsilon") {
        Regex r = Regex::epsilon();

        REQUIRE( accepts(r, {}) );
        REQUIRE( !accepts(r, {'a'}) );
        REQUIRE( !accepts(r, {'b'}) );
        REQUIRE( !accepts(r, {'a', 'b'}) );
    }

    SECTION("Literal") {

        SECTION("'a'") {
            Regex a = Regex::literal('a');

            REQUIRE( !accepts(a, {}) );
            REQUIRE( accepts(a, {'a'}) );
            REQUIRE( !accepts(a, {'b'}) );
            REQUIRE( !accepts(a, {'a', 'b'}) );
        }

        SECTION("'b'") {
            Regex b = Regex::literal('b');

            REQUIRE( !accepts(b, {}) );
            REQUIRE( !accepts(b, {'a'}) );
            REQUIRE( accepts(b, {'b'}) );
            REQUIRE( !accepts(b, {'a', 'b'}) );
        }
    }

    SECTION("Disjunction") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex e = Regex::epsilon();

        Regex ab = a | b;
        Regex r = ab | e;

        REQUIRE( !accepts(ab, {}) );
        REQUIRE( accepts(ab, {'a'}) );
        REQUIRE( accepts(ab, {'b'}) );
        REQUIRE( !accepts(ab, {'a', 'b'}) );


        REQUIRE( accepts(r, {}) );
        REQUIRE( accepts(r, {'a'}) );
        REQUIRE( accepts(r, {'b'}) );
        REQUIRE( !accepts(r, {'a', 'b'}) );
    }

    SECTION("Sequence") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex e = Regex::epsilon();

        Regex ab = a & b;
        Regex abe = ab & e;
        Regex aba = ab & a;

        REQUIRE( !accepts(ab, {}) );
        REQUIRE( !accepts(ab, {'a'}) );
        REQUIRE( !accepts(ab, {'b'}) );
        REQUIRE( accepts(ab, {'a', 'b'}) );
        REQUIRE( !accepts(ab, {'a', 'b', 'a'}) );

        REQUIRE( !accepts(abe, {}) );
        REQUIRE( !accepts(abe, {'a'}) );
        REQUIRE( !accepts(abe, {'b'}) );
        REQUIRE( accepts(abe, {'a', 'b'}) );
        REQUIRE( !accepts(abe, {'a', 'b', 'a'}) );

        REQUIRE( !accepts(aba, {}) );
        REQUIRE( !accepts(aba, {'a'}) );
        REQUIRE( !accepts(aba, {'b'}) );
        REQUIRE( !accepts(aba, {'a', 'b'}) );
        REQUIRE( accepts(aba, {'a', 'b', 'a'}) );
    }

    SECTION("Closure") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex c = Regex::literal('c');

        Regex r = *((a & b) | c);

        REQUIRE( accepts(r, {}) );
        REQUIRE( !accepts(r, {'a'}) );
        REQUIRE( !accepts(r, {'b'}) );
        REQUIRE( accepts(r, {'c'}) );
        REQUIRE( accepts(r, {'a', 'b'}) );
        REQUIRE( accepts(r, {'a', 'b', 'c'}) );
        REQUIRE( accepts(r, {'a', 'b', 'a', 'b'}) );
        REQUIRE( accepts(r, {'c', 'a', 'b', 'a', 'b', 'c'}) );
    }

}

TEMPLATE_TEST_CASE("Regex utilities input tests", "[template]", tfl::RegexesDerivation<char>) {
    using Regex = tfl::Regex<char>;
    using Regexes = tfl::Regexes<char>;

    auto accepts = [](Regex r, std::initializer_list<char> ls){ return TestType::accepts(r, ls); };

    SECTION("Word") {
        auto r = Regexes::word({'t', 'o', 'm', 'a', 't', 'o'});

        REQUIRE( !accepts(r, {}) );
        REQUIRE( accepts(r, {'t', 'o', 'm', 'a', 't', 'o'}) );
        REQUIRE( !accepts(r, {'o', 'm', 'a', 't', 'o'}) );
        REQUIRE( !accepts(r, {'t', 'o', 'm', 'a', 't'}) );
        REQUIRE( !accepts(r, {'t', 'o', 'm', 'a', 't', 'o', 'e', 's'}) );
        REQUIRE( !accepts(r, {'t', 'o', 'm', 'e', 't', 'o'}) );
    }

    SECTION("Any (literal)") {
        auto r = Regexes::any_of({'t', 'o', 'm', 'a'});

        REQUIRE( !accepts(r, {}) );
        REQUIRE( accepts(r, {'t'}) );
        REQUIRE( accepts(r, {'o'}) );
        REQUIRE( accepts(r, {'m'}) );
        REQUIRE( accepts(r, {'a'}) );
        REQUIRE( !accepts(r, {'t', 'o', 'm', 'a'}) );
        REQUIRE( !accepts(r, {'t', 'o', 'm', 'a', 't', 'o'}) );
    }

    SECTION("Range") {
        auto r = Regexes::range('2', '4');

        REQUIRE( !accepts(r, {'0'}) );
        REQUIRE( !accepts(r, {'1'}) );
        REQUIRE( accepts(r, {'2'}) );
        REQUIRE( accepts(r, {'3'}) );
        REQUIRE( accepts(r, {'4'}) );
        REQUIRE( !accepts(r, {'5'}) );
        REQUIRE( !accepts(r, {'6'}) );
        REQUIRE( !accepts(r, {'7'}) );
        REQUIRE( !accepts(r, {'8'}) );
        REQUIRE( !accepts(r, {'9'}) );
    }
}