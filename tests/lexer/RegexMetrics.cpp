#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;
using Size = tfl::RegexesMetrics<char>::Size;

static auto size = tfl::RegexesMetrics<char>::size;
static auto depth = tfl::RegexesMetrics<char>::depth;

static inline void test_size_depth(Regex const& regex, char const* name, Size exp_size, Size exp_depth) {
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

    test_size_depth(Regex::empty(), "∅", 1, 1);
    test_size_depth(Regex::epsilon(), "ε", 1, 1);
    test_size_depth(Regex::alphabet(), "Σ", 1, 1);
    test_size_depth(a, "a", 1, 1);
    test_size_depth(a, "b", 1, 1);
    test_size_depth(a, "c", 1, 1);
    test_size_depth(a, "d", 1, 1);

    test_size_depth(((a - b) | c | d), "(ab | c | d)", 7, 4);
    test_size_depth(~*~a, "¬*¬a", 4, 4);
    test_size_depth(*((a - b) | ~c | d), "*(ab | ¬c | d)", 9, 5);
    test_size_depth(*(a - b - ~c - d), "*(ab¬cd)", 9, 5);
    test_size_depth(*(~a - b - c - d), "*(¬abcd)", 9, 6);
    test_size_depth(*( (a - b) - (c - d) ), "*((ab)(cd))", 8, 4);
    test_size_depth(*( (a - b) - (~c - d) ), "*((ab)(¬cd))", 9, 5);
    test_size_depth(*( (a - b) & (~c - d) ), "*(ab&¬cd)", 9, 5);

}

TEST_CASE("Compacted regexes have expected size/depth") {
    auto e = Regex::epsilon();
    auto f = Regex::empty();
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');
    auto any = Regex::any();

    auto test_singleton = [](auto regex, auto name){ test_size_depth(regex, name, 1, 1); };
    auto test_dualton = [](auto regex, auto name){ test_size_depth(regex, name, 2, 2); };

    test_singleton(a | f, "a | ∅");
    test_singleton(f | a, "∅ | a");
    test_singleton(a - f, "a∅");
    test_singleton(f - a, "∅a");
    test_singleton(a - e, "aε");
    test_singleton(e - a, "εa");
    test_singleton(*e, "*ε");
    test_singleton(*f, "*∅");
    test_singleton(~~a, "¬¬a");
    test_singleton(a & f, "a & ∅");
    test_singleton(f & a, "∅ & a");
    test_singleton(any & f, "a & *Σ");
    test_singleton(f & any, "*Σ & a");

    test_dualton(**a, "**a");
    test_dualton(a | any, "a | *Σ");
    test_dualton(any | a, "*Σ | a");
}