#pragma once

#include "Automata.hpp"
#include "Regex.hpp"
#include "RegexOps.hpp"

#include <unordered_set>
#include <numeric>

/**
 * @brief Contains additional operations on automata. 
 * @file
 *
 * @note Since DFA/NFA are implicitely convertible
 * into each other, all the following functions
 * can be used with both indifferently.
 */

namespace tfl {

    /**
     * @brief Creates the cross product of two DFAs.
     * 
     * Given two (complete) DFAs \f$ L \f$ and \f$ R \f$, their cross product is defined as follows:
     * - \f$ Q = M(Q_L \times Q_R) \f$;
     * - \f$ \Sigma = \Sigma_L \cup \Sigma_R \f$;
     * - \f$ \delta_{L \times R}(M(l \times r), x) =  \delta_L(l, x) \times \delta_R(r, x)  \f$;
     * - \f$ q_0 = 0 \f$;
     * - \f$ F = \emptyset \f$.
     *
     * where \f$ M : Q_L \times Q_R \longmapsto \{0,1,\ldots,|Q_L| \cdot |Q_R| - 1, \textsc{DEAD}\} \f$
     *
     * Missing transitions will be set to go to the dead state.
     *
     * @tparam T Type of literals.
     *
     * @see \ref cross_map(), \ref cross_remap() for \f$ M \f$ and \f$ M^{-1} \f$
     */
    template<typename T>
    DFA<T>::Builder cross(typename DFA<T>::Builder const& left, typename DFA<T>::Builder const& right) {
        using StateIdx = DFA<T>::StateIdx;

        auto size = (left.state_count()+1) * (right.state_count()+1) - 1;
        auto compute_idx = cross_map<T>(left, right);
        auto compute_ids = cross_remap<T>(left, right);

        std::unordered_set<T> inputs;
        auto linputs = left.alphabet();
        auto rinputs = right.alphabet();
        inputs.insert(linputs.begin(), linputs.end());
        inputs.insert(rinputs.begin(), rinputs.end());

        typename DFA<T>::Builder builder(inputs, size);

        std::vector<StateIdx> lstates(left.state_count()+1);
        std::iota(lstates.begin(), lstates.end()-1, 0);
        lstates[left.state_count()] = DFA<T>::DEAD_STATE;
        
        std::vector<StateIdx> rstates(right.state_count()+1);
        std::iota(rstates.begin(), rstates.end()-1, 0);
        rstates[right.state_count()] = DFA<T>::DEAD_STATE;

        for(auto input: inputs) {
            for(StateIdx i = 0; i < builder.state_count(); ++i) {
                auto [l, r] = compute_ids(i);

                builder.set_transition(
                    i,
                    input,
                    compute_idx(left.transition(l, input), right.transition(r, input))
                );
            }
        }

        for(StateIdx i = 0; i < builder.state_count(); ++i) {
            auto [l, r] = compute_ids(i);

            builder.set_unknown_transition(
                i,
                compute_idx(left.unknown_transition(l), right.unknown_transition(r))
            );
        }

        builder.complete(DFA<T>::DEAD_STATE);

        return builder;
    }

    /**
     * @brief Returns \f$ M \f$.
     * @see \ref cross()
     */
    template<typename T>
    auto cross_map(typename DFA<T>::Builder const& left, typename DFA<T>::Builder const& right) {
        using StateIdx = DFA<T>::StateIdx;
        auto lsize = left.state_count()+1;
        auto rsize = right.state_count()+1;
        return [lsize, rsize](std::optional<StateIdx> const& l, std::optional<StateIdx> const& r){ 
            StateIdx lv = l.value_or(DFA<T>::DEAD_STATE);
            StateIdx rv = r.value_or(DFA<T>::DEAD_STATE);
            if(lv == DFA<T>::DEAD_STATE && rv == DFA<T>::DEAD_STATE) {
                return DFA<T>::DEAD_STATE;
            }
            else if(lv == DFA<T>::DEAD_STATE){
                return lsize-1 + rv*lsize;
            }
            else if(rv == DFA<T>::DEAD_STATE){
                return lv + (rsize-1)*lsize;
            }
            else {
                return lv + rv*lsize;
            }
        };
    }

    /**
     * @brief Returns \f$ M^{-1} \f$.
     * @see \ref cross()
     */
    template<typename T>
    auto cross_remap(typename DFA<T>::Builder const& left, typename DFA<T>::Builder const& right) {
        using StateIdx = DFA<T>::StateIdx;
        auto lsize = left.state_count()+1;
        auto rsize = right.state_count()+1;
        return [lsize, rsize](std::optional<StateIdx> const& s){ 
            StateIdx v = s.value_or(DFA<T>::DEAD_STATE);

            if(v == DFA<T>::DEAD_STATE) {
                return std::pair{DFA<T>::DEAD_STATE, DFA<T>::DEAD_STATE};
            }

            StateIdx l = v % lsize;
            StateIdx r = v / lsize;

            if(l == lsize-1) {
                l = DFA<T>::DEAD_STATE;
            }
            if(r == rsize-1) {
                r = DFA<T>::DEAD_STATE;
            }

            return std::pair{l, r};
        };
    }

    /**
     * @brief Creates a NFA which doesn't accept any string.
     * 
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder empty() {
        return typename NFA<T>::Builder(1);
    }

    /**
     * @brief Creates a NFA which only accepts ε.
     * 
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder epsilon() {
        return typename NFA<T>::Builder(1)
            .set_acceptance(0, true);
    }

    /**
     * @brief Creates a NFA which accepts any sequence of length 1 (i.e. whose language is Σ).
     * 
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder alphabet() {
        return typename NFA<T>::Builder(2)
            .add_unknown_transition(0, 1)
            .set_acceptance(1, true);
    }

    /**
     * @brief Creates a NFA which only accepts \f$ (t) \f$.
     * 
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder literal(T const& t) {
        return typename NFA<T>::Builder({t}, 2)
            .add_transition(0, t, 1)
            .set_acceptance(1, true);
    }

    /**
     * @brief Creates a NFA which is the disjunction of two others.
     * 
     * \f[ \mathcal{L} = \mathcal{L}(L) \cup \mathcal{L}(R) \f]
     *
     * @tparam T Type of literals.
     */
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

    /**
     * @brief Creates a NFA which is the sequencing of two others.
     * 
     * \f[ \mathcal{L} = \left\{ l \mathbin{++} r \mid l \in \mathcal{L}(L) \textup{ and } r \in \mathcal{L}(R) \right\} \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder sequence(typename NFA<T>::Builder const& left, typename NFA<T>::Builder const& right) {
        typename NFA<T>::Builder builder(left);
        auto r = builder.meld(right).second;

        for(typename NFA<T>::StateIdx i = 0; i < r; ++i) {
            if(builder.is_accepting(i)) {
                builder
                    .add_epsilon_transition(i, r)
                    .set_acceptance(i, false);
            }
        }

        return builder;
    }

    /**
     * @brief Creates a NFA which is the kleene closure of another.
     * 
     * If we define 
     * \f[ S^0 = \{\varepsilon\} \qquad S^{(i+1)} = \left\{ v \mathbin{++} w \mid v \in \mathcal{L}(N) \textup{ and } w \in S^i \right\} \f]
     * then
     * \f[ \mathcal{L} = \bigcup S^i \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T>::Builder closure(typename NFA<T>::Builder const& automaton) {
        typename NFA<T>::Builder builder;
        builder
            .add_state(true).first
            .meld(automaton).first
            .add_epsilon_transition(0, 1);

        for(typename NFA<T>::StateIdx i = 1; i < builder.state_count(); ++i) {
            if(builder.is_accepting(i)) {
                builder.add_epsilon_transition(i, 0);
            }
        }

        return builder;
    }

    /**
     * @brief Creates a NFA which is the complement of another.
     * 
     * \f[ \mathcal{L} = \Sigma^* \backslash \mathcal{L}(D) \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    DFA<T>::Builder complement(typename DFA<T>::Builder const& automaton) {
        typename DFA<T>::Builder builder(automaton);
        builder.complete(DFA<T>::DEAD_STATE);
        
        return builder.complement();
    }

    /**
     * @brief Creates a NFA which is the conujunction of two others.
     * 
     * \f[ \mathcal{L} = \mathcal{L}(L) \cap \mathcal{L}(R) \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    DFA<T>::Builder conjunction(typename DFA<T>::Builder const& left, typename DFA<T>::Builder const& right) {
        auto builder = cross<T>(left, right);
        auto compute_ids = cross_remap<T>(left, right);

        for(typename DFA<T>::StateIdx i = 0; i < builder.state_count(); ++i) {
            auto [l, r] = compute_ids(i);

            builder.set_acceptance(
                i,
                left.is_accepting(l) && right.is_accepting(r)
            );
        }

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

    /**
     * @brief Converts a regex into an equivalent NFA.
     * 
     * \f[ \mathcal{L} = \mathcal{L}(R) \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    NFA<T> make_nfa(Regex<T> const& regex) {
        return regex.match(regex_to_nfa<T>);
    }

    /**
     * @brief Converts a regex into an equivalent NFA.
     * 
     * \f[ \mathcal{L} = \mathcal{L}(R) \f]
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    DFA<T> make_dfa(Regex<T> const& regex) {
        return regex.match(regex_to_nfa<T>).make_deterministic();
    }
}