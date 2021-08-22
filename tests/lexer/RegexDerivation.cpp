#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;

auto nullable = tfl::RegexesDerivation<char>::nullable;

TEST_CASE("Base regexes have expected nullability") {
    CHECK( !nullable(Regex::empty()) );
    CHECK(  nullable(Regex::epsilon()) );
    CHECK( !nullable(Regex::literal('a')) );
}

TEST_CASE("Combinated regexes have expected nullability") {
    GIVEN("Two nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::epsilon();
        CHECK( nullable(l) );
        CHECK( nullable(r) );

        THEN("Their sequence is nullable") {
            CHECK( nullable(l & r) );
        }
        THEN("Their disjunction is nullable") {
            CHECK( nullable(l | r) );
        }
    }

    GIVEN("One nullable/ one non-nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::literal('a');
        CHECK(  nullable(l) );
        CHECK( !nullable(r) );

        THEN("Their sequences are non-nullable") {
            CHECK( !nullable(l & r) );
            CHECK( !nullable(r & l) );
        }
        THEN("Their disjunctions are nullable") {
            CHECK( nullable(l | r) );
            CHECK( nullable(r | l) );
        }
    }

    GIVEN("Two non-nullable regex") {
        Regex l = Regex::literal('a');
        Regex r = Regex::literal('a');
        CHECK( !nullable(l) );
        CHECK( !nullable(r) );

        THEN("Their sequence is non-nullable") {
            CHECK( !nullable(l & r) );
        }
        THEN("Their disjunction is non-nullable") {
            CHECK( !nullable(l | r) );
        }
    }
}

TEST_CASE("Regexes are nullable after closure") {
    GIVEN("A nullable regex") {
        Regex r = Regex::epsilon();
        CHECK( nullable(r) );

        THEN("Its closure is nullable") {
            CHECK( nullable(*r) );
        }
    }

    GIVEN("A non-nullable regex") {
        Regex r = Regex::literal('a');
        CHECK( !nullable(r) );

        THEN("Its closure is nullable") {
            CHECK( nullable(*r) );
        }
    }
}
