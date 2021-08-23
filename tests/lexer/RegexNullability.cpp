#include <catch2/catch_test_macros.hpp>

#include "tfl/Regex.hpp"

using Regex = tfl::Regex<char>;

auto is_nullable = tfl::is_nullable<char>;

TEST_CASE("Base regexes have expected nullability") {
    CHECK( !is_nullable(Regex::empty()) );
    CHECK(  is_nullable(Regex::epsilon()) );
    CHECK( !is_nullable(Regex::alphabet()) );
    CHECK( !is_nullable(Regex::literal('a')) );
}

TEST_CASE("Combined regexes have expected nullability") {
    GIVEN("Two is_nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::epsilon();
        CHECK( is_nullable(l) );
        CHECK( is_nullable(r) );

        THEN("Their sequence is is_nullable") {
            CHECK( is_nullable(l - r) );
        }
        THEN("Their disjunction is is_nullable") {
            CHECK( is_nullable(l | r) );
        }
        THEN("Their conjunction is is_nullable") {
            CHECK( is_nullable(l & r) );
        }
    }

    GIVEN("One is_nullable/ one non-is_nullable regex") {
        Regex l = Regex::epsilon();
        Regex r = Regex::literal('a');
        CHECK(  is_nullable(l) );
        CHECK( !is_nullable(r) );

        THEN("Their sequences are non-is_nullable") {
            CHECK( !is_nullable(l - r) );
            CHECK( !is_nullable(r - l) );
        }
        THEN("Their disjunctions are is_nullable") {
            CHECK( is_nullable(l | r) );
            CHECK( is_nullable(r | l) );
        }
        THEN("Their conjunctions are non-is_nullable") {
            CHECK( !is_nullable(l & r) );
            CHECK( !is_nullable(r & l) );
        }
    }

    GIVEN("Two non-is_nullable regex") {
        Regex l = Regex::literal('a');
        Regex r = Regex::literal('a');
        CHECK( !is_nullable(l) );
        CHECK( !is_nullable(r) );

        THEN("Their sequence is non-is_nullable") {
            CHECK( !is_nullable(l - r) );
        }
        THEN("Their disjunction is non-is_nullable") {
            CHECK( !is_nullable(l | r) );
        }
        THEN("Their conjunction is non-is_nullable") {
            CHECK( !is_nullable(l & r) );
        }
    }
}

TEST_CASE("Regexes are is_nullable after closure") {
    GIVEN("A is_nullable regex") {
        Regex r = Regex::epsilon();
        CHECK( is_nullable(r) );

        THEN("Its closure is is_nullable") {
            CHECK( is_nullable(*r) );
        }
    }

    GIVEN("A non-is_nullable regex") {
        Regex r = Regex::literal('a');
        CHECK( !is_nullable(r) );

        THEN("Its closure is is_nullable") {
            CHECK( is_nullable(*r) );
        }
    }
}

TEST_CASE("Complemented regexes have expected nullability") {
    GIVEN("A is_nullable regex") {
        Regex r = Regex::epsilon();
        CHECK( is_nullable(r) );

        THEN("Its complement is non-is_nullable") {
            CHECK( !is_nullable(~r) );
        }
    }

    GIVEN("A non-is_nullable regex") {
        Regex r = Regex::literal('a');
        CHECK( !is_nullable(r) );

        THEN("Its complement is is_nullable") {
            CHECK( is_nullable(~r) );
        }
    }
}
