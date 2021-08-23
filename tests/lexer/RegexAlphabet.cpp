#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;
static auto generate_minimal_alphabet = tfl::generate_minimal_alphabet<char>;
static auto to_string = tfl::to_string<char>;

void test(Regex const& regex, std::set<char> const& expected) {
    GIVEN(to_string(regex)) {
        THEN("Its minimal alphabet is " << Catch::StringMaker<std::set<char>>::convert(expected)) {
            CHECK( generate_minimal_alphabet(regex) == expected );
        }
    }
}

TEST_CASE("Regexes have expected minimal alphabet") {
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');
    auto c = Regex::literal('c');
    auto d = Regex::literal('d');
    auto e = Regex::epsilon();
    auto f = Regex::empty();
    auto any = Regex::any();

    SECTION("Base cases") {
        test(a, {'a'});
        test(b, {'b'});
        test(c, {'c'});
        test(d, {'d'});
        test(e, {});
        test(f, {});
        test(any, {});
    }

    SECTION("Binary combinators") {
        test(a-b-c-d, {'a', 'b', 'c', 'd'});
        test(a|b|c|d, {'a', 'b', 'c', 'd'});
        test(a&b&c&d, {'a', 'b', 'c', 'd'});
        test((a-b)|(c&d), {'a', 'b', 'c', 'd'});
    }

    SECTION("Unary combinators") {
        test(*(a-b-c-d), {'a', 'b', 'c', 'd'});
        test(a|*b|c|~d, {'a', 'b', 'c', 'd'});
        test(a&*(b&c)&d, {'a', 'b', 'c', 'd'});
    }

    SECTION("Compacted regexes") {
        test(f-a-b-c-d, {});
        test(a | (b-f) | (c-(d | any)) , {'a', 'c'});
    }

}