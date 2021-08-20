#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;
using Size = tfl::RegexesMetrics<char>::Size;

static auto size = tfl::RegexesMetrics<char>::size;
static auto depth = tfl::RegexesMetrics<char>::depth;

static void test_size_depth(Regex const& regex, char const* name, Size exp_size, Size exp_depth) {
    GIVEN(name) {
        THEN("Its size is " << exp_size) {
            CHECK( size(regex) == exp_size );
        }
        THEN("Its depth is " << exp_depth) {
            CHECK( depth(regex) == exp_depth );
        }
    }
}

TEST_CASE("Regexes have expected size/depth") {
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');
    auto c = Regex::literal('c');
    auto d = Regex::literal('d');

    test_size_depth(*((a & b) | c | d), "(ab|c|d)*", 8, 5);
    test_size_depth(*(a & b & c & d), "(abcd)*", 8, 5);
    test_size_depth(*( (a & b) & (c & d) ), "((ab)(cd))*", 8, 4);

}

TEST_CASE("Compacted regexes have expected size/depth") {
    auto e = Regex::epsilon();
    auto f = Regex::empty();
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');

    auto test_singleton = [](auto regex, auto name){ test_size_depth(regex, name, 1, 1); };

    test_singleton(a | f, "a|⊥");
    test_singleton(f | a, "⊥|a");
    test_singleton(a & f, "a⊥");
    test_singleton(f & a, "⊥a");
    test_singleton(a & e, "aε");
    test_singleton(e & a, "εa");
}