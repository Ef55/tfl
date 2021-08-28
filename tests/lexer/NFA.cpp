#include <catch2/catch_test_macros.hpp>

#include "tfl/Automata.hpp"

using DFA = tfl::DFA<char>;
using NFA = tfl::NFA<char>;

TEST_CASE("Basic NFAs can be built and used") {
    SECTION("L = ∅") {
        NFA nfa = NFA::Builder(1);

        CHECK( !nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { ε }") {
        NFA nfa = NFA::Builder(1)
            .set_acceptance(0, true);

        CHECK( nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a }") {
        NFA nfa = NFA::Builder({'a'}, 2)
            .add_transition(0, 'a', 1)
            .set_acceptance(1, true);

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'c'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { x | x ∈ Σ }") {
        NFA nfa = NFA::Builder(2)
            .add_unknown_transition(0, 1)
            .set_acceptance(1, true);

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a, b }") {
        NFA nfa = NFA::Builder({'a', 'b'}, 5)
            .add_epsilon_transitions(0, {1, 2})
            .add_transition(1, 'a', 3)
            .add_transition(2, 'b', 4)
            .set_acceptance({3, 4}, true);

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
        CHECK( !nfa.accepts({'z', 'b'}) );
        CHECK( !nfa.accepts({'b', 'a'}) );
        CHECK( !nfa.accepts({'z', 'z'}) );
    }

    SECTION("L = { ab }") {
        NFA nfa = NFA::Builder({'a', 'b'}, 4)
            .add_transition(0, 'a', 1)
            .add_epsilon_transition(1, 2)
            .add_transition(2, 'b', 3)
            .set_acceptance(3, true);

        CHECK( !nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'a', 'b'}) );
        CHECK( !nfa.accepts({'z', 'b'}) );
        CHECK( !nfa.accepts({'a', 'z'}) );
        CHECK( !nfa.accepts({'a', 'b', 'a'}) );
        CHECK( !nfa.accepts({'z', 'b', 'a'}) );
        CHECK( !nfa.accepts({'a', 'z', 'z'}) );
    }

    SECTION("L = Closure({ ab, c }, concatenation)") {
        NFA nfa = NFA::Builder({'a', 'b', 'c'}, 7)
            .add_epsilon_transition(0, 1)
            .add_epsilon_transitions(1, {2, 5})
            .add_transition(2, 'a', 3)
            .add_transition(3, 'b', 4)
            .add_transition(5, 'c', 6)
            .add_epsilon_transition(4, 0)
            .add_epsilon_transition(6, 0)
            .set_acceptance({0, 4, 6}, true);

        CHECK( nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'c'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( nfa.accepts({'a', 'b'}) );
        CHECK( nfa.accepts({'a', 'b', 'c'}) );
        CHECK( !nfa.accepts({'a', 'b', 'z'}) );
        CHECK( nfa.accepts({'a', 'b', 'a', 'b'}) );
        CHECK( !nfa.accepts({'a', 'b', 'z', 'a', 'b'}) );
        CHECK( !nfa.accepts({'c', 'a', 'b', 'a', 'c'}) );
        CHECK( nfa.accepts({'c', 'a', 'b', 'a', 'b', 'c'}) );
    }
}

TEST_CASE("Epsilon elimination doesn't change the NFA's behavior") {
    SECTION("L = ∅") {
        NFA nfa = NFA::Builder(1)
            .epsilon_elimination();

        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( !nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { ε }") {
        NFA nfa = NFA::Builder(1)
            .set_acceptance(0, true)
            .epsilon_elimination();

        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a }") {
        NFA nfa = NFA::Builder({'a'}, 2)
            .add_transition(0, 'a', 1)
            .set_acceptance(1, true)
            .epsilon_elimination();

        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'c'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { x | x ∈ Σ }") {
        NFA nfa = NFA::Builder(2)
            .add_unknown_transition(0, 1)
            .set_acceptance(1, true)
            .epsilon_elimination();

        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a, b }") {
        NFA nfa = NFA::Builder({'a', 'b'}, 5)
            .add_epsilon_transitions(0, {1, 2})
            .add_transition(1, 'a', 3)
            .add_transition(2, 'b', 4)
            .set_acceptance({3, 4}, true)
            .epsilon_elimination();

        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( !nfa.accepts({}) );
        CHECK( nfa.accepts({'a'}) );
        CHECK( nfa.accepts({'b'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( !nfa.accepts({'a', 'b'}) );
        CHECK( !nfa.accepts({'z', 'b'}) );
        CHECK( !nfa.accepts({'b', 'a'}) );
        CHECK( !nfa.accepts({'z', 'z'}) );
    }

    SECTION("L = { ab }") {
        NFA nfa = NFA::Builder({'a', 'b'}, 4)
            .add_transition(0, 'a', 1)
            .add_epsilon_transition(1, 2)
            .add_transition(2, 'b', 3)
            .set_acceptance(3, true)
            .epsilon_elimination();
            
        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( !nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'a', 'b'}) );
        CHECK( !nfa.accepts({'z', 'b'}) );
        CHECK( !nfa.accepts({'a', 'z'}) );
        CHECK( !nfa.accepts({'a', 'b', 'a'}) );
        CHECK( !nfa.accepts({'z', 'b', 'a'}) );
        CHECK( !nfa.accepts({'a', 'z', 'z'}) );
    }

    SECTION("L = Closure({ ab, c }, concatenation)") {
        NFA nfa = NFA::Builder({'a', 'b', 'c'}, 7)
            .add_epsilon_transition(0, 1)
            .add_epsilon_transitions(1, {2, 5})
            .add_transition(2, 'a', 3)
            .add_transition(3, 'b', 4)
            .add_transition(5, 'c', 6)
            .add_epsilon_transition(4, 0)
            .add_epsilon_transition(6, 0)
            .set_acceptance({0, 4, 6}, true)
            .epsilon_elimination();
            
        REQUIRE( !nfa.has_epsilon_transitions() );

        CHECK( nfa.accepts({}) );
        CHECK( !nfa.accepts({'a'}) );
        CHECK( !nfa.accepts({'b'}) );
        CHECK( nfa.accepts({'c'}) );
        CHECK( !nfa.accepts({'z'}) );
        CHECK( nfa.accepts({'a', 'b'}) );
        CHECK( nfa.accepts({'a', 'b', 'c'}) );
        CHECK( !nfa.accepts({'a', 'b', 'z'}) );
        CHECK( nfa.accepts({'a', 'b', 'a', 'b'}) );
        CHECK( !nfa.accepts({'a', 'b', 'z', 'a', 'b'}) );
        CHECK( !nfa.accepts({'c', 'a', 'b', 'a', 'c'}) );
        CHECK( nfa.accepts({'c', 'a', 'b', 'a', 'b', 'c'}) );
    }
}

TEST_CASE("NFAs can be converted into DFAs") {
    SECTION("L = ∅") {
        DFA dfa = NFA::Builder(1)
            .determinize();

        CHECK( !dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { ε }") {
        DFA dfa = NFA::Builder(1)
            .set_acceptance(0, true)
            .determinize();

        CHECK( dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a }") {
        DFA dfa = NFA::Builder({'a'}, 2)
            .add_transition(0, 'a', 1)
            .set_acceptance(1, true)
            .determinize();

        CHECK( !dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'c'}) );
        CHECK( !dfa.accepts({'z'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { x | x ∈ Σ }") {
        DFA dfa = NFA::Builder(2)
            .add_unknown_transition(0, 1)
            .set_acceptance(1, true)
            .determinize();

        CHECK( !dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'z'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a, b }") {
        DFA dfa = NFA::Builder({'a', 'b'}, 5)
            .add_epsilon_transitions(0, {1, 2})
            .add_transition(1, 'a', 3)
            .add_transition(2, 'b', 4)
            .set_acceptance({3, 4}, true)
            .determinize();

        CHECK( !dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'z'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
        CHECK( !dfa.accepts({'z', 'b'}) );
        CHECK( !dfa.accepts({'b', 'a'}) );
        CHECK( !dfa.accepts({'z', 'z'}) );
    }

    SECTION("L = { ab }") {
        DFA dfa = NFA::Builder({'a', 'b'}, 4)
            .add_transition(0, 'a', 1)
            .add_epsilon_transition(1, 2)
            .add_transition(2, 'b', 3)
            .set_acceptance(3, true)
            .determinize();

        CHECK( !dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'a', 'b'}) );
        CHECK( !dfa.accepts({'z', 'b'}) );
        CHECK( !dfa.accepts({'a', 'z'}) );
        CHECK( !dfa.accepts({'a', 'b', 'a'}) );
        CHECK( !dfa.accepts({'z', 'b', 'a'}) );
        CHECK( !dfa.accepts({'a', 'z', 'z'}) );
    }

    SECTION("L = Closure({ ab, c }, concatenation)") {
        DFA dfa = NFA::Builder({'a', 'b', 'c'}, 7)
            .add_epsilon_transition(0, 1)
            .add_epsilon_transitions(1, {2, 5})
            .add_transition(2, 'a', 3)
            .add_transition(3, 'b', 4)
            .add_transition(5, 'c', 6)
            .add_epsilon_transition(4, 0)
            .add_epsilon_transition(6, 0)
            .set_acceptance({0, 4, 6}, true)
            .determinize();

        CHECK( dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'c'}) );
        CHECK( !dfa.accepts({'z'}) );
        CHECK( dfa.accepts({'a', 'b'}) );
        CHECK( dfa.accepts({'a', 'b', 'c'}) );
        CHECK( !dfa.accepts({'a', 'b', 'z'}) );
        CHECK( dfa.accepts({'a', 'b', 'a', 'b'}) );
        CHECK( !dfa.accepts({'a', 'b', 'z', 'a', 'b'}) );
        CHECK( !dfa.accepts({'c', 'a', 'b', 'a', 'c'}) );
        CHECK( dfa.accepts({'c', 'a', 'b', 'a', 'b', 'c'}) );
    }
}