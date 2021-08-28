#include <catch2/catch_test_macros.hpp>

#include "tfl/Automata.hpp"

using DFA = tfl::DFA<char>;

TEST_CASE("Basic DFAs can be built and used") {
    SECTION("L = ∅") {
        DFA dfa = DFA::Builder(1)
            .set_unknown_transition(0, 0);

        CHECK( !dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { ε }") {
        DFA dfa = DFA::Builder(2)
            .set_unknown_transition(0, 1)
            .set_unknown_transition(1, 1)
            .set_acceptance(0, true);

        CHECK( dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a }") {
        DFA dfa = DFA::Builder({'a'}, 3)
            .set_transition(0, 'a', 1)
            .set_unknown_transition(0, 2)
            .set_all_transitions(1, 2)
            .set_all_transitions(2, 2)
            .set_acceptance(1, true);

        CHECK( !dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( !dfa.accepts({'b'}) );
        CHECK( !dfa.accepts({'c'}) );
        CHECK( !dfa.accepts({'z'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { x | x ∈ Σ }") {
        DFA dfa = DFA::Builder(3)
            .set_unknown_transition(0, 1)
            .set_unknown_transition(1, 2)
            .set_all_transitions(2, 2)
            .set_acceptance(1, true);

        CHECK( !dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'z'}) );
        CHECK( !dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a }^C") {
        DFA dfa = DFA::Builder({'a'}, 3)
            .set_transition(0, 'a', 1)
            .set_unknown_transition(0, 2)
            .set_all_transitions(1, 2)
            .set_all_transitions(2, 2)
            .set_acceptance({0, 2}, true);

        CHECK( dfa.accepts({}) );
        CHECK( !dfa.accepts({'a'}) );
        CHECK( dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'c'}) );
        CHECK( dfa.accepts({'z'}) );
        CHECK( dfa.accepts({'a', 'b'}) );
    }

    SECTION("L = { a, b }") {
        DFA dfa = DFA::Builder({'a', 'b'}, 3)
            .set_transitions(0, {{'a', 1}, {'b', 1}})
            .set_unknown_transition(0, 2)
            .set_all_transitions(1, 2)
            .set_all_transitions(2, 2)
            .set_acceptance(1, true);

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
        DFA dfa = DFA::Builder({'a', 'b'}, 4)
            .set_transitions(0, {{'a', 1}, {'b', 3}})
            .set_unknown_transition(0, 3)
            .set_transitions(1, {{'a', 3}, {'b', 2}})
            .set_unknown_transition(1, 3)
            .set_all_transitions(2, 3)
            .set_all_transitions(3, 3)
            .set_acceptance(2, true);

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
        DFA dfa = DFA::Builder({'a', 'b', 'c'}, 4)
            .set_transitions(0, {{'a', 1}, {'b', 3}, {'c', 2}})
            .set_transitions(1, {{'a', 3}, {'b', 2}, {'c', 3}})
            .set_transitions(2, {{'a', 1}, {'b', 3}, {'c', 2}})
            .set_unknown_transition(0, 3)
            .set_unknown_transition(1, 3)
            .set_unknown_transition(2, 3)
            .set_all_transitions(3, 3)
            .set_acceptance({0, 2}, true);

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

TEST_CASE("Incomplete DFA cannot be built") {
    SECTION("Defaulted one") {
        CHECK_THROWS(
            DFA::Builder(1).finalize()
        );
    }

    SECTION("Missing unknown") {
        CHECK_THROWS(
            DFA::Builder(2)
                .set_unknown_transition(0, 1)
                .set_acceptance(0, true)
                .finalize()
        );
    }

    SECTION("Missing transition") {
        CHECK_THROWS(
            DFA::Builder({'a'}, 1)
                .set_unknown_transition(0, 0)
                .finalize()
        );

        CHECK_THROWS(
            DFA::Builder({'a', 'b'}, 1)
                .set_transition(0, 'a', 0)
                .set_unknown_transition(0, 0)
                .finalize()
        );
    }
}

TEST_CASE("DFA quirks") {
    SECTION("Unknown transitions are copied on input addition") {
        DFA dfa = DFA::Builder(1)
            .set_unknown_transition(0, 0)
            .add_input('a')
            .set_acceptance(0, true);

        
        CHECK( dfa.accepts({}) );
        CHECK( dfa.accepts({'a'}) );
        CHECK( dfa.accepts({'b'}) );
        CHECK( dfa.accepts({'a', 'b'}) );
    }
}