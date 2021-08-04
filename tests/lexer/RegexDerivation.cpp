#include <catch2/catch.hpp>

#include "lexer/Regex.hpp"

using Regex = tfl::Regex<char>;

TEST_CASE("Base cases nullability") {
    REQUIRE( !Regex::empty().nullable() );
    REQUIRE(  Regex::epsilon().nullable() );
    REQUIRE( !Regex::literal('a').nullable() );
}

TEST_CASE("Combinators nullability") {
    GIVEN("Two nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::epsilon();
        REQUIRE( l.nullable() );
        REQUIRE( r.nullable() );

        THEN("Their sequence is nullable") {
            REQUIRE( (l & r).nullable() );
        }
        THEN("Their disjunction is nullable") {
            REQUIRE( (l | r).nullable() );
        }
    }

    GIVEN("One nullable/ one non-nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::literal('a');
        REQUIRE(  l.nullable() );
        REQUIRE( !r.nullable() );

        THEN("Their sequences are non-nullable") {
            REQUIRE( !(l & r).nullable() );
            REQUIRE( !(r & l).nullable() );
        }
        THEN("Their disjunctions are nullable") {
            REQUIRE( (l | r).nullable() );
            REQUIRE( (r | l).nullable() );
        }
    }

    GIVEN("Two non-nullable regex") {
        Regex l = Regex::literal('a');
        Regex r = Regex::literal('a');
        REQUIRE( !l.nullable() );
        REQUIRE( !r.nullable() );

        THEN("Their sequence is non-nullable") {
            REQUIRE( !(l & r).nullable() );
        }
        THEN("Their disjunction is non-nullable") {
            REQUIRE( !(l | r).nullable() );
        }
    }
}

TEST_CASE("Closure nullability") {
    GIVEN("A nullable regex") {
        Regex r = Regex::epsilon();
        REQUIRE( r.nullable() );

        THEN("Its closure is nullable") {
            REQUIRE( (*r).nullable() );
        }
    }

    GIVEN("A non-nullable regex") {
        Regex r = Regex::literal('a');
        REQUIRE( !r.nullable() );

        THEN("Its closure is nullable") {
            REQUIRE( (*r).nullable() );
        }
    }
}