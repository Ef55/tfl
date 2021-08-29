#pragma once

#include "Automata.hpp"
#include "Regex.hpp"
#include "RegexOps.hpp"

#include <unordered_set>

namespace tfl {

    template<typename T>
    NFA<T>::Builder empty() {
        return typename NFA<T>::Builder(1);
    }

    template<typename T>
    NFA<T>::Builder epsilon() {
        return typename NFA<T>::Builder(1)
            .set_acceptance(0, true);
    }

    template<typename T>
    NFA<T>::Builder alphabet() {
        return typename NFA<T>::Builder(2)
            .add_unknown_transition(0, 1)
            .set_acceptance(1, true);
    }
    
    template<typename T>
    NFA<T>::Builder literal(T const& value) {
        return typename NFA<T>::Builder({value}, 2)
            .add_transition(0, value, 1)
            .set_acceptance(1, true);
    }

    template<typename T>
    NFA<T>::Builder disjunction(typename NFA<T>::Builder const& left, typename NFA<T>::Builder const& right) {
        typename NFA<T>::Builder builder(1);
        auto l = builder.meld(left).second;
        auto r = builder.meld(right).second;

        builder
            .add_epsilon_transition(0, l)
            .add_epsilon_transition(0, r);

        return builder;
    }

    template<typename T>
    NFA<T>::Builder sequence(typename NFA<T>::Builder const& left, typename NFA<T>::Builder const& right) {
        typename NFA<T>::Builder builder(left);
        auto r = builder.meld(right).second;

        for(typename NFA<T>::StateIdx i = 0; i < r; ++i) {
            if(builder.accepting_states()[i]) {
                builder
                    .add_epsilon_transition(i, r)
                    .set_acceptance(i, false);
            }
        }

        return builder;
    }

    template<typename T>
    NFA<T>::Builder closure(typename NFA<T>::Builder const& automaton) {
        typename NFA<T>::Builder builder;
        builder
            .add_state(true).first
            .meld(automaton).first
            .add_epsilon_transition(0, 1);

        for(typename NFA<T>::StateIdx i = 1; i < builder.state_count(); ++i) {
            if(builder.accepting_states()[i]) {
                builder.add_epsilon_transition(i, 0);
            }
        }

        return builder;
    }

    template<typename T>
    DFA<T>::Builder complement(typename DFA<T>::Builder const& automaton) {
        typename DFA<T>::Builder builder(automaton);
        if(!builder.is_complete()) {
            typename NFA<T>::StateIdx trash = builder.add_state().second;
            builder.complete(trash);
        }
        
        return builder.complement();
    }

    template<typename T>
    DFA<T>::Builder conjunction(typename DFA<T>::Builder const& left, typename DFA<T>::Builder const& right) {
        using StateIdx = DFA<T>::StateIdx;

        auto size = left.state_count() * right.state_count() + 1;
        auto trash = size-1;
        auto lsize = left.state_count();
        auto compute_idx = [lsize, trash](std::optional<StateIdx> const& l, std::optional<StateIdx> const& r){ 
            return (l.has_value() && r.has_value()) ?
                l.value() + r.value()*lsize :
                trash;
        };

        std::unordered_set<T> inputs;
        auto linputs = left.inputs();
        auto rinputs = right.inputs();
        inputs.insert(linputs.begin(), linputs.end());
        inputs.insert(rinputs.begin(), rinputs.end());

        typename DFA<T>::Builder builder(inputs, size);
        
        for(auto input: inputs) {
            auto& ltransitions = left.transitions(input);
            auto& rtransitions = right.transitions(input);

            for(StateIdx l = 0; l < left.state_count(); ++l) {
                for(StateIdx r = 0; r < right.state_count(); ++r) {
                    builder.set_transition(
                        compute_idx(l, r),
                        input,
                        compute_idx(ltransitions[l], rtransitions[r])
                    );
                }
            }
        }

        for(StateIdx l = 0; l < left.state_count(); ++l) {
            for(StateIdx r = 0; r < right.state_count(); ++r) {
                builder.set_unknown_transition(
                    compute_idx(l, r),
                    compute_idx(left.unknown_transitions()[l], right.unknown_transitions()[r])
                );

                builder.set_acceptance(
                    compute_idx(l, r),
                    left.accepting_states()[l] && right.accepting_states()[r]
                );
            }
        }

        builder.complete(trash);

        return builder;
    }

    namespace {
        template<typename T>
        class RegexToNFA: public matchers::Base<T, typename NFA<T>::Builder> {
            using Builder = NFA<T>::Builder;
            using matchers::Base<T, Builder>::rec;
        public:
            Builder empty() const {
                return tfl::empty<T>();
            }
            Builder epsilon() const {
                return tfl::epsilon<T>();
            }
            Builder alphabet() const {
                return tfl::alphabet<T>();
            }
            Builder literal(T const& literal) const {
                return tfl::literal(literal);
            }
            Builder disjunction(Regex<T> const& left, Regex<T> const& right) const {
                return tfl::disjunction<T>(rec(left), rec(right));
            }
            Builder sequence(Regex<T> const& left, Regex<T> const& right) const {
                return tfl::sequence<T>(rec(left), rec(right));
            }
            Builder kleene_star(Regex<T> const& regex) const {
                return tfl::closure<T>(rec(regex));
            }
            Builder complement(Regex<T> const& regex) const {
                return tfl::complement<T>(rec(regex));
            }
            Builder conjunction(Regex<T> const& left, Regex<T> const& right) const {
                return tfl::conjunction<T>(rec(left), rec(right));
            }
        };
        template<typename T> constexpr RegexToNFA<T> regex_to_nfa{};

    }

    template<typename T>
    NFA<T> make_nfa(Regex<T> const& regex) {
        return regex.match(regex_to_nfa<T>);
    }

    template<typename T>
    DFA<T> make_dfa(Regex<T> const& regex) {
        return regex.match(regex_to_nfa<T>).make_deterministic();
    }
}