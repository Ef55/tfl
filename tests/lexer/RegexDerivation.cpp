#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;

auto nullable = tfl::RegexesDerivation<char>::nullable;

TEST_CASE("Base regexes have expected nullability") {
    REQUIRE( !nullable(Regex::empty()) );
    REQUIRE(  nullable(Regex::epsilon()) );
    REQUIRE( !nullable(Regex::literal('a')) );
}

TEST_CASE("Combinated regexes have expected nullability") {
    GIVEN("Two nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::epsilon();
        REQUIRE( nullable(l) );
        REQUIRE( nullable(r) );

        THEN("Their sequence is nullable") {
            REQUIRE( nullable(l & r) );
        }
        THEN("Their disjunction is nullable") {
            REQUIRE( nullable(l | r) );
        }
    }

    GIVEN("One nullable/ one non-nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::literal('a');
        REQUIRE(  nullable(l) );
        REQUIRE( !nullable(r) );

        THEN("Their sequences are non-nullable") {
            REQUIRE( !nullable(l & r) );
            REQUIRE( !nullable(r & l) );
        }
        THEN("Their disjunctions are nullable") {
            REQUIRE( nullable(l | r) );
            REQUIRE( nullable(r | l) );
        }
    }

    GIVEN("Two non-nullable regex") {
        Regex l = Regex::literal('a');
        Regex r = Regex::literal('a');
        REQUIRE( !nullable(l) );
        REQUIRE( !nullable(r) );

        THEN("Their sequence is non-nullable") {
            REQUIRE( !nullable(l & r) );
        }
        THEN("Their disjunction is non-nullable") {
            REQUIRE( !nullable(l | r) );
        }
    }
}

TEST_CASE("Regexes are nullable after closure") {
    GIVEN("A nullable regex") {
        Regex r = Regex::epsilon();
        REQUIRE( nullable(r) );

        THEN("Its closure is nullable") {
            REQUIRE( nullable(*r) );
        }
    }

    GIVEN("A non-nullable regex") {
        Regex r = Regex::literal('a');
        REQUIRE( !nullable(r) );

        THEN("Its closure is nullable") {
            REQUIRE( nullable(*r) );
        }
    }
}
