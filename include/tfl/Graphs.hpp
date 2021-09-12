#pragma once

#include <ostream>

#include "RegexOps.hpp"
#include "Automata.hpp"

/**
 * @brief Contains functions to generate graphs.
 * @file
 */

namespace {
    static constexpr char const* NORMAL_STATE_SHAPE = "circle";
    static constexpr char const* ACCEPTING_STATE_SHAPE = "doublecircle";

    template<typename T>
    struct GraphWrapper final {
        T const& inner;
    };

    template<typename T>
    class RegexDotGrapher final: public tfl::matchers::regex::MutableBase<T, std::size_t> {
    private:
        using tfl::matchers::regex::MutableBase<T, std::size_t>::rec;

        std::ostream& _stream;
        std::size_t _id;

        inline std::size_t new_id() { return _id++; }

        inline std::size_t leaf(std::string const& label) {
            auto id = new_id();
            _stream << id << "[shape=none,label=<<u>" << label << "</u>>];\n";
            return id;
        }

        inline std::size_t unary(std::string const& label, tfl::Regex<T> const&  child) {
            auto c = rec(child);
            auto id = new_id();
            _stream << id << "[shape=none,label=\"" << label << "\"];\n";
            _stream << id << " -> " << c << ";\n";
            return id;
        }

        inline std::size_t binary(std::string const& label, tfl::Regex<T> const& left, tfl::Regex<T> const& right) {
            auto l = rec(left);
            auto r = rec(right);
            auto id = new_id();
            _stream << id << "[shape=none,label=\"" << label << "\"];\n"; 
            _stream << id << " -> " << l << ";\n";
            _stream << id << " -> " << r << ";\n";
            return id;
        }

    public:
        RegexDotGrapher(std::ostream& stream): _stream(stream), _id(0) {}

        std::size_t empty() override {
            return leaf("∅");
        }
        std::size_t epsilon() override {
            return leaf("ε");
        }
        std::size_t alphabet() override {
            return leaf("Σ");
        }
        std::size_t literal(T const& literal) override {
            return leaf(tfl::Stringify<T>::convert(literal));
        }
        std::size_t disjunction(tfl::Regex<T> const& left, tfl::Regex<T> const& right) override {
            return binary("|", left, right);
        }
        std::size_t sequence(tfl::Regex<T> const& left, tfl::Regex<T> const& right) override {
            return binary("·", left, right);
        }
        std::size_t kleene_star(tfl::Regex<T> const& regex) override {
            return unary("*", regex);
        }
        std::size_t complement(tfl::Regex<T> const& regex) override {
            return unary("¬", regex);
        }
        std::size_t conjunction(tfl::Regex<T> const& left, tfl::Regex<T> const& right) override {
            return binary("&", left, right);
        }
    };
}

namespace tfl {

    /**
     * @name Dot graphs generation
     * @brief Generates a dot graph which can then be outputed.
     * 
     * The graph is outputed as plaintext in <a href="https://www.graphviz.org/about/">Graphviz/Dot</a> format.
     *
     * @note These functions are only used to indicate that the subsequent
     * call to `operator<<` should output a graph. This implies that the actual
     * computation of the graph only happens when `operator<<` is called, and that
     * the return of this function should not be used in any other way 
     * (e.g. it should not be stored inside a variable).
     * \code{.cpp}
     * // Correct usage:
     * std::cout << dot_graph(regex) << std::endl;
     * \endcode
     *
     * @{
     */

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
    ///@}
}

/**
 * @name Graphs output
 * @brief Outputs a graph to a stream.
 * @{
 */

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

    for(auto input: dfa.alphabet()) {
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

    for(auto input: nfa.alphabet()) {
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
///@}