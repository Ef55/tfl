#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "tfl/Regex.hpp"


using Regex = tfl::Regex<char>;
using Regexes = tfl::Regexes<char>;
 
TEMPLATE_TEST_CASE("Regexes accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
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

TEMPLATE_TEST_CASE("Compacted regexes accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
    auto accepts = [](Regex r, std::vector<char> ls){ return TestType::accepts(r, ls.cbegin(), ls.cend()); };

    auto e = Regex::epsilon();
    auto f = Regex::empty();
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');

    std::vector<std::vector<char>> inputs = {
        {},
        {'a'},
        {'b'},
        {'a', 'b'}
    };

    auto test = [&inputs, &accepts](auto compacted, auto compacted_name, auto expected, auto expected_name){
        GIVEN(compacted_name) {

            THEN("it behaves as " << expected_name) {
                for(auto input: inputs) {
                    CHECK( accepts(compacted, input) == accepts(expected, input) );
                }
            }
        } 
    };

    test(a | f, "a|⊥", a, "a");
    test(f | a, "⊥|a", a, "a");
    test(a & f, "a⊥", f, "⊥");
    test(f & a, "⊥a", f, "⊥");
    test(a & e, "aε", a, "a");
    test(e & a, "εa", a, "a");
}

TEMPLATE_TEST_CASE("Additional regex combinators accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
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