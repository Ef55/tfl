#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;
using Regexes = tfl::Regexes<char>;

struct RegexesDerivation {
    static bool accepts(Regex const& r, std::initializer_list<char> ls) {
        return tfl::is_nullable(tfl::derive(ls.begin(), ls.end(), r));
    }
    static bool accepts_v(Regex const& r, std::vector<char> ls) {
        return tfl::is_nullable(tfl::derive(ls.cbegin(), ls.cend(), r));
    }
};

#define ACCEPTERS RegexesDerivation

static auto to_string = tfl::to_string<char>;

static Regex const a = Regex::literal('a');
static Regex const b = Regex::literal('b');
static Regex const c = Regex::literal('c');
static Regex const e = Regex::epsilon();
static Regex const s = Regex::alphabet();
static Regex const any = ~Regex::empty();
 
TEMPLATE_TEST_CASE("Regexes accept/reject as expected", "[template]", ACCEPTERS) {
    auto accepts = TestType::accepts;
  
    SECTION("∅ (empty)") {
        Regex r = Regex::empty();

        CHECK( !accepts(r, {}) );
        CHECK( !accepts(r, {'a'}) );
        CHECK( !accepts(r, {'b'}) );
        CHECK( !accepts(r, {'a', 'b'}) );
    }

    SECTION("ε (epsilon)") {
        Regex r = Regex::epsilon();

        CHECK( accepts(r, {}) );
        CHECK( !accepts(r, {'a'}) );
        CHECK( !accepts(r, {'b'}) );
        CHECK( !accepts(r, {'a', 'b'}) );
    }

    SECTION("Σ (alphabet)") {
        Regex r = Regex::alphabet();

        CHECK( !accepts(r, {}) );
        CHECK( accepts(r, {'a'}) );
        CHECK( accepts(r, {'b'}) );
        CHECK( accepts(r, {'z'}) );
        CHECK( !accepts(r, {'a', 'b'}) );
    }

    SECTION("Literal (literal)") {

        SECTION("'a'") {
            CHECK( !accepts(a, {}) );
            CHECK( accepts(a, {'a'}) );
            CHECK( !accepts(a, {'b'}) );
            CHECK( !accepts(a, {'c'}) );
            CHECK( !accepts(a, {'z'}) );
            CHECK( !accepts(a, {'a', 'b'}) );
        }

        SECTION("'b'") {
            CHECK( !accepts(b, {}) );
            CHECK( !accepts(b, {'a'}) );
            CHECK( accepts(b, {'b'}) );
            CHECK( !accepts(b, {'c'}) );
            CHECK( !accepts(b, {'z'}) );
            CHECK( !accepts(b, {'a', 'b'}) );
        }

        SECTION("'c'") {
            CHECK( !accepts(c, {}) );
            CHECK( !accepts(c, {'a'}) );
            CHECK( !accepts(c, {'b'}) );
            CHECK( accepts(c, {'c'}) );
            CHECK( !accepts(c, {'z'}) );
            CHECK( !accepts(c, {'a', 'b'}) );
        }
    }

    SECTION("Disjunction (|)") {
        SECTION("a | b") {
            Regex ab = a | b;
            CHECK( !accepts(ab, {}) );
            CHECK( accepts(ab, {'a'}) );
            CHECK( accepts(ab, {'b'}) );
            CHECK( !accepts(ab, {'z'}) );
            CHECK( !accepts(ab, {'a', 'b'}) );
            CHECK( !accepts(ab, {'z', 'b'}) );
            CHECK( !accepts(ab, {'b', 'a'}) );
            CHECK( !accepts(ab, {'z', 'z'}) );
        }

        SECTION("a | b | ε") {
            Regex r = a | b | e;
            CHECK( accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( accepts(r, {'b'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( !accepts(r, {'a', 'b'}) );
            CHECK( !accepts(r, {'z', 'b'}) );
            CHECK( !accepts(r, {'b', 'a'}) );
            CHECK( !accepts(r, {'z', 'z'}) );
        }
    }

    SECTION("Sequence (-)") {
        SECTION("ab") {
            Regex ab = a - b;
            CHECK( !accepts(ab, {}) );
            CHECK( !accepts(ab, {'a'}) );
            CHECK( !accepts(ab, {'b'}) );
            CHECK( accepts(ab, {'a', 'b'}) );
            CHECK( !accepts(ab, {'z', 'b'}) );
            CHECK( !accepts(ab, {'a', 'z'}) );
            CHECK( !accepts(ab, {'a', 'b', 'a'}) );
            CHECK( !accepts(ab, {'z', 'b', 'a'}) );
            CHECK( !accepts(ab, {'a', 'z', 'z'}) );
        }

        SECTION("abε") {
            Regex abe = a - b - e;
            CHECK( !accepts(abe, {}) );
            CHECK( !accepts(abe, {'a'}) );
            CHECK( !accepts(abe, {'b'}) );
            CHECK( accepts(abe, {'a', 'b'}) );
            CHECK( !accepts(abe, {'z', 'b'}) );
            CHECK( !accepts(abe, {'a', 'z'}) );
            CHECK( !accepts(abe, {'a', 'b', 'a'}) );
            CHECK( !accepts(abe, {'z', 'b', 'a'}) );
            CHECK( !accepts(abe, {'a', 'z', 'z'}) );
        }

        SECTION("a(ba)") {
            Regex aba = a - (b - a);
            CHECK( !accepts(aba, {}) );
            CHECK( !accepts(aba, {'a'}) );
            CHECK( !accepts(aba, {'b'}) );
            CHECK( !accepts(aba, {'a', 'b'}) );
            CHECK( !accepts(aba, {'z', 'b'}) );
            CHECK( !accepts(aba, {'a', 'z'}) );
            CHECK( accepts(aba, {'a', 'b', 'a'}) );
            CHECK( !accepts(aba, {'z', 'b', 'a'}) );
            CHECK( !accepts(aba, {'a', 'z', 'z'}) );
        }

        SECTION("ΣbΣ") {
            Regex sbs = s - b - s;
            CHECK( !accepts(sbs, {}) );
            CHECK( !accepts(sbs, {'a'}) );
            CHECK( !accepts(sbs, {'b'}) );
            CHECK( !accepts(sbs, {'a', 'b'}) );
            CHECK( !accepts(sbs, {'z', 'b'}) );
            CHECK( !accepts(sbs, {'a', 'z'}) );
            CHECK( accepts(sbs, {'a', 'b', 'a'}) );
            CHECK( accepts(sbs, {'b', 'b', 'b'}) );
            CHECK( accepts(sbs, {'z', 'b', 'a'}) );
            CHECK( !accepts(sbs, {'a', 'z', 'z'}) );
            CHECK( !accepts(sbs, {'b', 'z', 'b'}) );
        }
    }

    SECTION("Closure (*)") {
        SECTION("*a") {
            Regex r = *a;
            CHECK( accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( !accepts(r, {'c'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( accepts(r, {'a', 'a'}) );
            CHECK( !accepts(r, {'a', 'b', 'a'}) );
            CHECK( !accepts(r, {'a', 'b', 'z'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a', 'a', 'a'}) );
        }

        SECTION("*(ab | c)") {
            Regex r = *((a - b) | c);
            CHECK( accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( accepts(r, {'c'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( accepts(r, {'a', 'b'}) );
            CHECK( accepts(r, {'a', 'b', 'c'}) );
            CHECK( !accepts(r, {'a', 'b', 'z'}) );
            CHECK( accepts(r, {'a', 'b', 'a', 'b'}) );
            CHECK( !accepts(r, {'a', 'b', 'z', 'a', 'b'}) );
            CHECK( !accepts(r, {'c', 'a', 'b', 'a', 'c'}) );
            CHECK( accepts(r, {'c', 'a', 'b', 'a', 'b', 'c'}) );
        }
    }

    SECTION("Complement (~)") {
        SECTION("¬a") {
            Regex na = ~a;
            CHECK( accepts(na, {}) );
            CHECK( !accepts(na, {'a'}) );
            CHECK( accepts(na, {'b'}) );
            CHECK( accepts(na, {'a', 'a'}) );
        }

        SECTION("¬¬a") {
            Regex nna = ~~a;
            CHECK( !accepts(nna, {}) );
            CHECK( accepts(nna, {'a'}) );
            CHECK( !accepts(nna, {'b'}) );
            CHECK( !accepts(nna, {'a', 'b'}) );
        }

        SECTION("¬∅") {
            CHECK( accepts(any, {}) );
            CHECK( accepts(any, {'a'}) );
            CHECK( accepts(any, {'b'}) );
            CHECK( accepts(any, {'a', 'b'}) );
        }

        SECTION("¬(a | b)") {
            Regex r = ~(a | b);
            CHECK( accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( accepts(r, {'z'}) );
            CHECK( accepts(r, {'a', 'b'}) );
            CHECK( accepts(r, {'z', 'b'}) );
            CHECK( accepts(r, {'b', 'a'}) );
            CHECK( accepts(r, {'z', 'z'}) );
        }

        SECTION("¬ab | a") {
            Regex r = (~a-b) | a;
            CHECK( !accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( accepts(r, {'b'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( !accepts(r, {'a', 'b'}) );
            CHECK( accepts(r, {'c', 'b'}) );
            CHECK( accepts(r, {'z', 'b'}) );
            CHECK( !accepts(r, {'b', 'a'}) );
            CHECK( !accepts(r, {'z', 'z'}) );
            CHECK( accepts(r, {'a', 'a', 'b'}) );
            CHECK( accepts(r, {'z', 'z', 'z', 'b'}) );
        }
    }

    SECTION("Conjunction (&)") {
        SECTION("a & b") {
            Regex r = a & b;
            CHECK( !accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( !accepts(r, {'a', 'b'}) );
            CHECK( !accepts(r, {'z', 'b'}) );
            CHECK( !accepts(r, {'b', 'a'}) );
            CHECK( !accepts(r, {'z', 'z'}) );
        }

        SECTION("a¬∅ & ¬∅b") {
            Regex r = a-any & any-b;
            CHECK( !accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( !accepts(r, {'z'}) );
            CHECK( accepts(r, {'a', 'b'}) );
            CHECK( !accepts(r, {'z', 'b'}) );
            CHECK( !accepts(r, {'b', 'a'}) );
            CHECK( !accepts(r, {'z', 'z'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'b'}) );
            CHECK( accepts(r, {'a', 'b', 'b', 'b'}) );
            CHECK( !accepts(r, {'b', 'a', 'a', 'a'}) );
            CHECK( !accepts(r, {'b', 'b', 'b', 'a'}) );
            CHECK( accepts(r, {'a', 'c', 'z', 'b'}) );
            CHECK( accepts(r, {'a', 'z', 'z', 'z', 'b'}) );
        }

        SECTION("*a & ¬ε & ¬(aa)") {
            Regex r = *a & 
                ~Regex::epsilon() & 
                ~(a - a);
            CHECK( !accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( !accepts(r, {'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'}) );
        }
    }

}

TEMPLATE_TEST_CASE("Compacted regexes accept/reject as expected", "[template]", ACCEPTERS) {
    auto accepts = TestType::accepts_v;

    auto e = Regex::epsilon();
    auto f = Regex::empty();
    auto any = Regex::any();
    auto any2 = *Regex::alphabet();
    auto a = Regex::literal('a');
    auto b = Regex::literal('b');

    auto test_fun = [&accepts](auto inputs, auto compacted, auto compacted_name, auto expected){
        GIVEN(compacted_name) {
            THEN("it behaves as " << to_string(expected)) {
                for(auto input: inputs) {
                    INFO( "Input: " << std::string(input.cbegin(), input.cend()) );
                    CHECK( accepts(compacted, input) == accepts(expected, input) );
                }
            }
        } 
    };


    SECTION("Sequence/disjunction/conjunction with empty/epsilon"){
        std::vector<std::vector<char>> inputs = {
            {},
            {'a'},
            {'b'},
            {'a', 'b'}
        };
        auto test = [&inputs, &test_fun](auto compacted, auto compacted_name, auto expected){ 
            test_fun(inputs, compacted, compacted_name, expected); 
        };

        test(a - f, "a∅", f);
        test(f - a, "∅a", f);
        test(a - e, "aε", a);
        test(e - a, "εa", a);
        test(a | f, "a | ∅", a);
        test(f | a, "∅ | a", a);
        test(a & f, "a & ∅", f);
        test(f & a, "∅ & a", f);
    }

    SECTION("Disjunction/conjunction with any"){
        std::vector<std::vector<char>> inputs = {
            {},
            {'a'},
            {'b'},
            {'a', 'b'}
        };
        auto test = [&inputs, &test_fun](auto compacted, auto compacted_name, auto expected){ 
            test_fun(inputs, compacted, compacted_name, expected); 
        };

        test(a | any, "a | ¬∅", any);
        test(any | a, "¬∅ | a", any);
        test(a & any, "a & ¬∅", a);
        test(any & a, "¬∅ & a", a);
        test(a | any2, "a | *Σ", any);
        test(any2 | a, "*Σ | a", any);
        test(a & any2, "a & *Σ", a);
        test(any2 & a, "*Σ & a", a);
    }

    SECTION("Closure"){
        std::vector<std::vector<char>> inputs = {
            {},
            {'a'},
            {'a', 'a', 'a'},
            {'b', 'a', 'a'},
            {'a', 'b', 'a'},
            {'a', 'a', 'a', 'a'},
        };
        auto test = [&inputs, &test_fun](auto compacted, auto compacted_name, auto expected){ 
            test_fun(inputs, compacted, compacted_name, expected); 
        };

        test(*e, "*ε", e);
        test(*f, "*∅", e);
        test(**a, "**a", *a);
    }
}

TEMPLATE_TEST_CASE("Additional regex combinators accept/reject as expected", "[template]", ACCEPTERS) {
    auto accepts = TestType::accepts;

    SECTION("Kleene + (+)") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex c = Regex::literal('c');

        Regex r = +((a - b) | c);

        SECTION("+(ab | c)") {
            CHECK( !accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( accepts(r, {'c'}) );
            CHECK( accepts(r, {'a', 'b'}) );
            CHECK( accepts(r, {'a', 'b', 'c'}) );
            CHECK( accepts(r, {'a', 'b', 'a', 'b'}) );
            CHECK( accepts(r, {'c', 'a', 'b', 'a', 'b', 'c'}) );
        }
    }

    SECTION("Substraction (/)") {
        Regex r = *Regex::literal('a') /
            Regex::epsilon() / 
            (Regex::literal('a') - Regex::literal('a'));

        SECTION("*a / ε / aa") {
            CHECK( !accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( !accepts(r, {'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'}) );
        }
    }

    SECTION("Optional (opt)") {
        auto r = Regexes::opt(Regexes::literal('a'));

        CHECK( accepts(r, {}) );
        CHECK( accepts(r, {'a'}) );
        CHECK( !accepts(r, {'b'}) );
    } 

    SECTION("Word (word)") {
        auto r = Regexes::word({'t', 'o', 'm', 'a', 't', 'o'});

        CHECK( !accepts(r, {}) );
        CHECK( accepts(r, {'t', 'o', 'm', 'a', 't', 'o'}) );
        CHECK( !accepts(r, {'o', 'm', 'a', 't', 'o'}) );
        CHECK( !accepts(r, {'t', 'o', 'm', 'a', 't'}) );
        CHECK( !accepts(r, {'t', 'o', 'm', 'a', 't', 'o', 'e', 's'}) );
        CHECK( !accepts(r, {'t', 'o', 'm', 'e', 't', 'o'}) );
    }

    SECTION("Any (any)") {
        auto r = Regexes::any();


        CHECK( accepts(r, {}) );
        CHECK( accepts(r, {'a'}) );
        CHECK( accepts(r, {'b'}) );
        CHECK( accepts(r, {'a', 'a', 'a'}) );
        CHECK( accepts(r, {'t', 'o', 'm', 'a', 't', 'o'}) );
    }

    SECTION("Any of (any_of)") {
        auto r = Regexes::any_of({'t', 'o', 'm', 'a'});

        CHECK( !accepts(r, {}) );
        CHECK( accepts(r, {'t'}) );
        CHECK( accepts(r, {'o'}) );
        CHECK( accepts(r, {'m'}) );
        CHECK( accepts(r, {'a'}) );
        CHECK( !accepts(r, {'t', 'o', 'm', 'a'}) );
        CHECK( !accepts(r, {'t', 'o', 'm', 'a', 't', 'o'}) );
    }

    SECTION("Range") {
        auto r = Regexes::range('2', '4');

        CHECK( !accepts(r, {'0'}) );
        CHECK( !accepts(r, {'1'}) );
        CHECK( accepts(r, {'2'}) );
        CHECK( accepts(r, {'3'}) );
        CHECK( accepts(r, {'4'}) );
        CHECK( !accepts(r, {'5'}) );
        CHECK( !accepts(r, {'6'}) );
        CHECK( !accepts(r, {'7'}) );
        CHECK( !accepts(r, {'8'}) );
        CHECK( !accepts(r, {'9'}) );
    }
}