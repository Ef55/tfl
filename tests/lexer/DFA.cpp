#include <catch2/catch_test_macros.hpp>

#include "tfl/Automata.hpp"

using DFA = tfl::DFA<char>;

TEST_CASE("Can be built using a builder") {
    DFA::Builder builder(3);
    builder.add_input('a');
    builder.set_transition(0, 'a', 1);
    builder.set_unknown_transition(0, 2);
    builder.set_transition(1, 'a', 2);
    builder.set_unknown_transition(1, 2);
    builder.set_transition(2, 'a', 2);
    builder.set_unknown_transition(2, 2);
    builder.set_acceptance(1, true);

    DFA a = builder;

    CHECK( !a.accepts({}) );
    CHECK( a.accepts({'a'}) );
    CHECK( !a.accepts({'b'}) );
    CHECK( !a.accepts({'a', 'a'}) );
    CHECK( !a.accepts({'a', 'a', 'a'}) );
}