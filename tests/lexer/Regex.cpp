#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "tfl/Regex.hpp"


using Regex = tfl::Regex<char>;
using Regexes = tfl::Regexes<char>;

static auto to_string = tfl::RegexesPrinter<char>::to_string;
 
TEMPLATE_TEST_CASE("Regexes accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
    auto accepts = [](Regex r, std::initializer_list<char> ls){ return TestType::accepts(r, ls); };
  
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

    SECTION("Literal (literal)") {

        SECTION("'a'") {
            Regex a = Regex::literal('a');

            CHECK( !accepts(a, {}) );
            CHECK( accepts(a, {'a'}) );
            CHECK( !accepts(a, {'b'}) );
            CHECK( !accepts(a, {'a', 'b'}) );
        }

        SECTION("'b'") {
            Regex b = Regex::literal('b');

            CHECK( !accepts(b, {}) );
            CHECK( !accepts(b, {'a'}) );
            CHECK( accepts(b, {'b'}) );
            CHECK( !accepts(b, {'a', 'b'}) );
        }
    }

    SECTION("Disjunction (|)") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex e = Regex::epsilon();

        Regex ab = a | b;
        Regex r = ab | e;

        SECTION("a | b") {
            CHECK( !accepts(ab, {}) );
            CHECK( accepts(ab, {'a'}) );
            CHECK( accepts(ab, {'b'}) );
            CHECK( !accepts(ab, {'a', 'b'}) );
        }

        SECTION("a | b | ε") {
            CHECK( accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( accepts(r, {'b'}) );
            CHECK( !accepts(r, {'a', 'b'}) );
        }
    }

    SECTION("Sequence (-)") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex e = Regex::epsilon();

        Regex ab = a - b;
        Regex abe = ab - e;
        Regex aba = ab - a;

        SECTION("ab") {
            CHECK( !accepts(ab, {}) );
            CHECK( !accepts(ab, {'a'}) );
            CHECK( !accepts(ab, {'b'}) );
            CHECK( accepts(ab, {'a', 'b'}) );
            CHECK( !accepts(ab, {'a', 'b', 'a'}) );
        }

        SECTION("abε") {
            CHECK( !accepts(abe, {}) );
            CHECK( !accepts(abe, {'a'}) );
            CHECK( !accepts(abe, {'b'}) );
            CHECK( accepts(abe, {'a', 'b'}) );
            CHECK( !accepts(abe, {'a', 'b', 'a'}) );
        }

        SECTION("aba") {
            CHECK( !accepts(aba, {}) );
            CHECK( !accepts(aba, {'a'}) );
            CHECK( !accepts(aba, {'b'}) );
            CHECK( !accepts(aba, {'a', 'b'}) );
            CHECK( accepts(aba, {'a', 'b', 'a'}) );
        }
    }

    SECTION("Closure (*)") {
        Regex a = Regex::literal('a');
        Regex b = Regex::literal('b');
        Regex c = Regex::literal('c');

        Regex r = *((a - b) | c);

        SECTION("*(ab | c)") {
            CHECK( accepts(r, {}) );
            CHECK( !accepts(r, {'a'}) );
            CHECK( !accepts(r, {'b'}) );
            CHECK( accepts(r, {'c'}) );
            CHECK( accepts(r, {'a', 'b'}) );
            CHECK( accepts(r, {'a', 'b', 'c'}) );
            CHECK( accepts(r, {'a', 'b', 'a', 'b'}) );
            CHECK( accepts(r, {'c', 'a', 'b', 'a', 'b', 'c'}) );
        }
    }

    SECTION("Complement (~)") {
        Regex a = Regex::literal('a');

        SECTION("¬a") {
            Regex na = ~a;
            CHECK( accepts(na, {}) );
            CHECK( !accepts(na, {'a'}) );
            CHECK( accepts(na, {'b'}) );
            CHECK( accepts(na, {'a', 'a'}) );
        }

        SECTION("¬¬a") {
            Regex nna = ~~a;
            CHECK( !accepts(a, {}) );
            CHECK( accepts(a, {'a'}) );
            CHECK( !accepts(a, {'b'}) );
            CHECK( !accepts(a, {'a', 'b'}) );
        }
    }

    SECTION("Conjunction (&)") {
        Regex r = *Regex::literal('a') & 
            ~Regex::epsilon() & 
            ~(Regex::literal('a') - Regex::literal('a'));

        SECTION("*a & ¬ε & ¬(aa)") {
            CHECK( !accepts(r, {}) );
            CHECK( accepts(r, {'a'}) );
            CHECK( !accepts(r, {'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a'}) );
            CHECK( accepts(r, {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'}) );
        }
    }

}

TEMPLATE_TEST_CASE("Compacted regexes accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
    auto accepts = [](Regex r, std::vector<char> ls){ return TestType::accepts(r, ls.cbegin(), ls.cend()); };

    auto e = Regex::epsilon();
    auto f = Regex::empty();
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


    SECTION("Sequence/disjunction with empty/epsilon"){
        std::vector<std::vector<char>> inputs = {
            {},
            {'a'},
            {'b'},
            {'a', 'b'}
        };
        auto test = [&inputs, &test_fun](auto compacted, auto compacted_name, auto expected){ 
            test_fun(inputs, compacted, compacted_name, expected); 
        };

        test(a | f, "a | ∅", a);
        test(f | a, "∅ | a", a);
        test(a - f, "a∅", f);
        test(f - a, "∅a", f);
        test(a - e, "aε", a);
        test(e - a, "εa", a);
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

TEMPLATE_TEST_CASE("Additional regex combinators accept/reject as expected", "[template]", tfl::RegexesDerivation<char>) {
    auto accepts = [](Regex r, std::initializer_list<char> ls){ return TestType::accepts(r, ls); };

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