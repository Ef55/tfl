#pragma once

#include <ostream>

#include "RegexOps.hpp"
#include "Automata.hpp"

namespace {
    static constexpr char const* NORMAL_STATE_SHAPE = "circle";
    static constexpr char const* ACCEPTING_STATE_SHAPE = "doublecircle";

    template<typename T>
    struct GraphWrapper final {
        T const& inner;
    };
}

namespace tfl {

    template<typename T>
    auto dot_graph(DFA<T> const& dfa) {
        return GraphWrapper<DFA<T>>(dfa);
    }

    template<typename T>
    auto dot_graph(NFA<T> const& nfa) {
        return GraphWrapper<NFA<T>>(nfa);
    }
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, GraphWrapper<tfl::DFA<T>> const& wdfa) {
    using StateIdx = tfl::DFA<T>::StateIdx;
    tfl::DFA<T> const& dfa(wdfa.inner);

    stream << "digraph {\n";

    for(StateIdx i = 0; i < dfa.state_count(); ++i) {
        stream << i << "[shape=" << (dfa.is_accepting(i) ? ACCEPTING_STATE_SHAPE : NORMAL_STATE_SHAPE) << "];\n";
    }
    stream << tfl::DFA<T>::DEAD_STATE << "[shape=" << NORMAL_STATE_SHAPE << ",label=∅];\n";
    stream << tfl::DFA<T>::DEAD_STATE << " -> " << tfl::DFA<T>::DEAD_STATE << ";\n";

    for(auto input: dfa.inputs()) {
        for(StateIdx i = 0; i < dfa.state_count(); ++i) { 
            stream << i << " -> " << dfa.transition(i, input) << "[label=" << tfl::Stringify<T>::convert(input) << "];\n";
        }
    }
    for(StateIdx i = 0; i < dfa.state_count(); ++i) {
        stream << i << " -> " << dfa.unknown_transition(i) << "[label=\"?\"];\n";
    }

    stream << "}";

    return stream;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, GraphWrapper<tfl::NFA<T>> const& wnfa) {
    using StateIdx = tfl::DFA<T>::StateIdx;
    tfl::NFA<T> const& nfa(wnfa.inner);

    stream << "digraph {\n";

    for(StateIdx i = 0; i < nfa.state_count(); ++i) {
        stream << i << "[shape=" << (nfa.is_accepting(i) ? ACCEPTING_STATE_SHAPE : NORMAL_STATE_SHAPE) << "];\n";
    }

    for(auto input: nfa.inputs()) {
        for(StateIdx i = 0; i < nfa.state_count(); ++i) { 
            for(StateIdx j: nfa.transition(i, input)) {
                stream << i << " -> " << j << "[label=" << tfl::Stringify<T>::convert(input) << "];\n";
            }
        }
    }
    for(StateIdx i = 0; i < nfa.state_count(); ++i) {
        for(StateIdx j: nfa.unknown_transition(i)) {
            stream << i << " -> " << j << "[label=\"?\"];\n";
        }

        for(StateIdx j: nfa.epsilon_transition(i)) {
            stream << i << " -> " << j << "[label=\"ε\"];\n";
        }
    }

    stream << "}";

    return stream;
}