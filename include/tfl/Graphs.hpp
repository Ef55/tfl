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

    struct RegexDotGrapherData {
        std::ostream& stream;
        std::size_t id;
    };

    template<typename T>
    class RegexDotGrapher final: public tfl::matchers::MutableBase<T, std::size_t, RegexDotGrapherData> {
    private:
        using tfl::matchers::Base<T, std::size_t>::rec;

        inline std::ostream& stream() const { return this->mut().stream; }
        inline std::size_t new_id() const { return this->mut().id++; }

        inline std::size_t leaf(std::string const& label) const {
            auto id = new_id();
            stream() << id << "[shape=none,label=<<u>" << label << "</u>>];\n";
            return id;
        }

        inline std::size_t unary(std::string const& label, tfl::Regex<T> const&  child) const {
            auto c = rec(child);
            auto id = new_id();
            stream() << id << "[shape=none,label=\"" << label << "\"];\n";
            stream() << id << " -> " << c << ";\n";
            return id;
        }

        inline std::size_t binary(std::string const& label, tfl::Regex<T> const& left, tfl::Regex<T> const& right) const {
            auto l = rec(left);
            auto r = rec(right);
            auto id = new_id();
            stream() << id << "[shape=none,label=\"" << label << "\"];\n"; 
            stream() << id << " -> " << l << ";\n";
            stream() << id << " -> " << r << ";\n";
            return id;
        }

    public:
        RegexDotGrapher(std::ostream& stream): tfl::matchers::MutableBase<T, std::size_t, RegexDotGrapherData>(stream, 0) {}

        std::size_t empty() const override {
            return leaf("∅");
        }
        std::size_t epsilon() const override {
            return leaf("ε");
        }
        std::size_t alphabet() const override {
            return leaf("Σ");
        }
        std::size_t literal(T const& literal) const override {
            return leaf(tfl::Stringify<T>::convert(literal));
        }
        std::size_t disjunction(tfl::Regex<T> const& left, tfl::Regex<T> const& right) const override {
            return binary("|", left, right);
        }
        std::size_t sequence(tfl::Regex<T> const& left, tfl::Regex<T> const& right) const override {
            return binary("·", left, right);
        }
        std::size_t kleene_star(tfl::Regex<T> const& regex) const override {
            return unary("*", regex);
        }
        std::size_t complement(tfl::Regex<T> const& regex) const override {
            return unary("¬", regex);
        }
        std::size_t conjunction(tfl::Regex<T> const& left, tfl::Regex<T> const& right) const override {
            return binary("&", left, right);
        }
    };
}

namespace tfl {

    template<typename T>
    auto dot_graph(Regex<T> const& regex) {
        return GraphWrapper<Regex<T>>(regex);
    }

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
std::ostream& operator<<(std::ostream& stream, GraphWrapper<tfl::Regex<T>> const& wregex) {
    stream << "digraph {\n";
    wregex.inner.match(RegexDotGrapher<T>(stream));
    stream << '}';

    return stream;
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
            stream << i << " -> " << dfa.transition(i, input) << "[label=\"" << tfl::Stringify<T>::convert(input) << "\"];\n";
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
                stream << i << " -> " << j << "[label=\"" << tfl::Stringify<T>::convert(input) << "\"];\n";
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