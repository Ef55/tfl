#pragma once

#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <optional>
#include <algorithm>
#include <set>
#include <queue>
#include <limits>

#include "tfl/Stringify.hpp"

/**
 * @brief Contains the definition of DFAs and NFAs.
 * @file
 */

namespace tfl {

    namespace {
        using namespace std::ranges;
    }

    template<typename T>
    class NFA;

    /**
     * @brief <a href=https://en.wikipedia.org/wiki/Deterministic_finite_automaton>DFA</a>.
     * 
     * This implementation uses a definition slightly different than the classic one.
     * A DFA is defined as a 5-tuple consisting of:
     * - The states \f$ Q = [0,1,\ldots,n, \textsc{DEAD}] \f$;
     * - The alphabet \f$ \Sigma = T^{-} \cup \textsc{UNKNOWN} \f$;
     * - The transition function \f$ \delta: Q \times \Sigma \longmapsto Q \f$;
     * - The initial state \f$ q_0 = 0 \f$;
     * - The accepting states \f$ F \subset Q \f$.
     *
     * where \f$ T^{-} \subset T \f$.
     *
     * Even though \f$ \Sigma \not= T \f$, the automata works with any sequence
     * in \f$ T^{*} \f$ by mapping every value in \f$ x \in T \f$ to \f$ x \f$
     * if \f$ x \in T^{-} \f$ and to \f$ \textsc{UNKNOWN} \f$ otherwise.
     *
     * The differences with the standard definition are:
     * - The states are designed by (unsigned) integers: this isn't a huge limitation and makes it more efficient;
     * - The dead state: is defined as an additional special state to allow early return when matching a string;
     * - The unknown transition and \f$ T^{-} \f$: allows to define the automaton on all values of type `T` without specifying every value;
     * - Default initial state: the initial state is always the state 0. Sorry but this was convenient.
     *
     * The DFA defines a language 
     * \f$ \mathcal{L} = \left\{ w \in \Sigma^* \mid \Delta(0 \times w) \in F \right\} \f$
     * where \f$ \Delta: Q \times \Sigma^* \longmapsto Q \f$ is the extended transition function
     * defined by
     * - \f$ \Delta(i \times \varepsilon) = i \f$;
     * - \f$ \Delta(i \times x \mathbin{::} w) = \Delta(\delta(i \times x) \times w) \f$.
     *
     * @tparam T Type of the alphabet.
     */
    template<typename T>
    class DFA final {
    public:
        /** 
         * @brief Type used to represent states. 
         * @hideinitializer
         */
        using StateIdx = std::size_t;

        /** 
         * @brief Index of the dead state. 
         * @hideinitializer
         */
        static constexpr StateIdx const DEAD_STATE = std::numeric_limits<StateIdx>::max();

    private:
        std::unordered_map<T, std::vector<StateIdx>> _transitions;
        std::vector<StateIdx> _unknown_transitions;
        std::vector<bool> _accepting_states;


        static bool is_special_state(StateIdx const& state) {
            return state >= DEAD_STATE;
        }

        StateIdx const& check_state(StateIdx const& state) const {
            if(state >= state_count() && !is_special_state(state)) {
                throw std::invalid_argument("Invalid state: " + std::to_string(state));
            }
            return state;
        }

        StateIdx const& check_ns_state(StateIdx const& state) const {
            if(state >= state_count()) {
                throw std::invalid_argument("Invalid non-special state: " + std::to_string(state));
            }
            return state;
        }

        StateIdx transition_unchecked(StateIdx const& state, T const& x) const {
            if(state == DEAD_STATE) {
                return DEAD_STATE;
            }

            auto it =  _transitions.find(x);
            if(it != _transitions.cend()) {
                return it->second[state];
            }
            else {
                return _unknown_transitions[state];
            }
        }

        template<range Tr, range Ut, range As>
        DFA(Tr&& transitions, Ut&& unknown_transitions, As&& accepting_states): 
        _transitions(std::ranges::cbegin(transitions), std::ranges::cend(transitions)), 
        _unknown_transitions(std::ranges::cbegin(unknown_transitions), std::ranges::cend(unknown_transitions)), 
        _accepting_states(std::ranges::cbegin(accepting_states), std::ranges::cend(accepting_states)) {
            
            auto s = _unknown_transitions.size();
            auto check = [&s](auto& ls, std::string const& name){ 
                if(ls.size() != s) {
                    throw std::invalid_argument("Table size mismatch: " + name + '.');
                }
            };

            check(_accepting_states, "accepting states");
            for(auto p: _transitions) {
                check(p.second, Stringify<T>::convert(p.first));
            }
        }

    public:
        /**
         * @brief Returns the number of states. The dead state is not counted.
         */
        StateIdx state_count() const {
            return _unknown_transitions.size();
        }

        /**
         * @brief Tests whether \f$ \textup{state} \in F\f$.
         */
        bool is_accepting(StateIdx const& state) const {
            return (state == DEAD_STATE) ? false : _accepting_states[check_ns_state(state)];
        }

        /**
         * @brief Returns \f$ \delta(\textup{state} \times x) \f$.
         *
         * @exception std::invalid_argument If \f$ x \not\in Q \f$
         * or \f$ x \not\in T^{-} \f$.
         */
        StateIdx transition(StateIdx const& state, T const& x) const {
            if(state == DEAD_STATE) {
                return DEAD_STATE;
            }

            check_ns_state(state);

            auto it =  _transitions.find(x);
            if(it != _transitions.cend()) {
                return it->second[state];
            }
            else {
                throw std::invalid_argument("Invalid input: " + Stringify<T>::convert(x));
            }
        }

        /**
         * @brief Returns \f$ \delta(\textup{state} \times \textsc{UNKNOWN}) \f$.
         * @exception std::invalid_argument If \f$ x \not\in Q \f$
         */
        StateIdx unknown_transition(StateIdx const& state) const {
            if(state == DEAD_STATE) {
                return DEAD_STATE;
            }

            return _unknown_transitions[check_ns_state(state)];
        }

        /**
         * @brief Returns \f$ T^{-} \f$.
         */
        auto alphabet() const {
            return transform_view(_transitions, [](auto p){ return p.first; });
        }

        /**
         * @name Language membership
         * @{
         */
        /**
         * @brief Tests whether \f$ \textup{sequence} \in \mathcal{L} \f$
         * 
         * @tparam R Type of the input sequence.
         * @param sequence The sequence to test for language-membership.
         */
        template<range_of<T> R>
        bool accepts(R&& sequence) const noexcept {
            StateIdx state = 0;
            for(
                auto beg = std::ranges::cbegin(sequence), end = std::ranges::cend(sequence); 
                (beg != end) && (state != DEAD_STATE); 
                ++beg
            ) {
                state = transition_unchecked(state, *beg);
            }

            return is_accepting(state);
        }

        /**
         * @brief Tests whether \f$ \textup{sequence} \in \mathcal{L} \f$
         * @see \ref accepts<R>()
         */
        bool accepts(std::initializer_list<T> sequence) const noexcept {
            return accepts(views::all(sequence));
        }

        /**
         * @brief Find the length of the longest prefix belonging to \f$ \mathcal{L} \f$.
         *
         * Formally, given a sequence \f$ (x_1, x_2, \ldots, x_n) \f$ returns 
         * \f$ \max \left\{ l \mid (x_1, x_2, \ldots, x_l) \in \mathcal{L} \right\} \f$
         * 
         * @tparam R Type of the input sequence.
         * @param sequence The sequence to munch.
         * @return Empty if no prefix belongs to the language, otherwise the length of the longest prefix.
         *
         * @note The returned length might be 0 if \f$ \varepsilon \in \mathcal{L} \f$.
         */
        template<range_of<T> R>
        std::optional<std::size_t> munch(R&& sequence) const noexcept {
            StateIdx state = 0;
            std::size_t step = 0;
            std::optional<std::size_t> res = is_accepting(state) ? 
                std::optional<std::size_t>{0} : 
                std::optional<std::size_t>{std::nullopt};

            for(
                auto beg = std::ranges::cbegin(sequence), end = std::ranges::cend(sequence); 
                (beg != end) && (state != DEAD_STATE); 
                ++beg
            ) {
                ++step;
                state = transition_unchecked(state, *beg);

                if(is_accepting(state)) {
                    res = step;
                }
            }

            return res;
        }

        /**
         * @brief Find the length of the longest prefix belonging to \f$ \mathcal{L} \f$.
         * @see \ref munch<R>()
         */
        std::optional<std::size_t> munch(std::initializer_list<T> sequence) const noexcept {
            return munch(views::all(sequence));
        }
        ///@}

        /**
         * @brief Allows DFA creation.
         */
        class Builder final {
            using OptStateIdx = std::optional<StateIdx>;

            std::unordered_map<T, std::vector<OptStateIdx>> _transitions;
            std::vector<OptStateIdx> _unknown_transitions;
            std::vector<bool> _accepting_states;

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count() && !is_special_state(state)) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
                }
                return state;
            }

            StateIdx const& check_ns_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid non-special state: " + std::to_string(state));
                }
                return state;
            }

            T const& check_input(T const& input) const {
                if(!_transitions.contains(input)) {
                    throw std::invalid_argument("Invalid input: " + Stringify<T>::convert(input));
                }
                return input;
            }

        public:
            /**
             * @brief Creates a builder with initial \f$ T^{-} = \textup{inputs}\f$ and `size` states.
             */
            template<range_of<T> R>
            Builder(R&& inputs, StateIdx size = 0): _transitions(), _unknown_transitions(size, std::nullopt), _accepting_states(size, 0) {
                for(auto input: inputs) {
                    add_input(input);
                }
            }

            /**
             * @brief Creates a builder with initial \f$ T^{-} = \textup{inputs}\f$ and `size` states.
             */
            Builder(std::initializer_list<T> inputs = {}, StateIdx size = 0): Builder(views::all(inputs), size) {}
            
            /**
             * @brief Creates a builder with `size` states.
             */
            Builder(StateIdx size): Builder(views::empty<T>, size) {}

            /**
             * @brief Returns the number of states. The dead state is not counted.
             */
            StateIdx state_count() const {
                return _unknown_transitions.size();
            }

            /**
             * @brief Tests whether \f$ \textup{state} \in F\f$.
             */
            bool is_accepting(StateIdx const& state) const {
                return state == DEAD_STATE ? false : _accepting_states[check_ns_state(state)];
            }

            /**
             * @name Transition getters 
             * @{
             */
            /**
             * @brief Returns \f$ \delta(\textup{state} \times x) \f$.
             *
             * If \f$ x \not\in T^{-} \f$, then this is equivalent to \ref unknown_transition().
             *
             * @return Empty if the transition is not yet defined, the transition otherwise.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$.
             */
            std::optional<StateIdx> transition(StateIdx const& state, T const& x) const {
                if(state == DEAD_STATE) {
                    return std::optional{DEAD_STATE};
                }

                check_ns_state(state);

                auto it =  _transitions.find(x);
                if(it != _transitions.cend()) {
                    return it->second[state];
                }
                else {
                    return _unknown_transitions[state];
                }
            }

            /**
             * @brief Returns \f$ \delta(\textup{state} \times \textsc{UNKNOWN}) \f$.
             * 
             * @return Empty if the transition is not yet defined, the transition otherwise.
             */
            std::optional<StateIdx> unknown_transition(StateIdx const& state) const {
                return (state == DEAD_STATE) ? std::optional{DEAD_STATE} : _unknown_transitions[check_ns_state(state)];
            }
            ///@}

            /**
             * @brief Returns \f$ T^{-} \f$.
             */
            auto alphabet() const {
                return transform_view(_transitions, [](auto p){ return p.first; });
            }

            /**
             * @brief Adds \f$ t \f$ to \f$ T^{-} \f$.
             * @return This.
             */
            Builder& add_input(T const& t) {
                _transitions.insert(std::pair{
                    t, 
                    _unknown_transitions
                });
                return *this;
            }

            /**
             * @brief Adds a new state \f$ i \f$.
             * 
             * @param to The (optional) value of \f$ \delta(i, x) \forall x \in (\Sigma \cup \textsc{UNKNOWN}) \f$.
             * @param accepting whether \f$ i \in F \f$.
             * @return  This, as well as the new state's index, \f$ i \f$.
             * @exception std::invalid_argument If \f$ \textup{to} \not\in Q \f$
             */
            std::pair<Builder&, StateIdx> add_state(std::optional<StateIdx> const& to = std::nullopt, bool accepting = false) {
                if(is_special_state(state_count())) {
                    throw std::logic_error("DFA reached maximal size.");
                }

                if(to.has_value()) {
                    check_state(to.value());
                }

                for(auto& it: _transitions) {
                    it.second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _accepting_states.push_back(accepting);
                return { *this, state_count()-1 };
            }

            /**
             * @name Acceptance setters
             * @return This.
             * @{
             */
            /**
             * @brief Sets whether \f$ \textup{state} \in F \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$.
             */
            Builder& set_acceptance(StateIdx const& state, bool value) {
                check_ns_state(state);
                _accepting_states[state] = value;
                return *this;
            }

            /**
             * @brief Sets whether \f$ \textup{states} \subset F \f$.
             * @exception std::invalid_argument If \f$ \textup{states} \not\subset Q \f$.
             */
            template<range_of<StateIdx> R>
            Builder& set_acceptance(R&& states, bool value) {
                for(auto state: states) {
                    check_ns_state(state);
                    _accepting_states[state] = value;
                }
                return *this;
            }

            /**
             * @brief Sets whether \f$ \textup{states} \subset F \f$.
             * @exception std::invalid_argument If \f$ \textup{states} \not\subset Q \f$.
             */
            Builder& set_acceptance(std::initializer_list<StateIdx> states, bool value) {
                return set_acceptance(views::all(states), value);
            }
            ///@}

            /**
             * @name Transition setters
             * @return This.
             * @{
             */
            /**
             * @brief Sets \f$ \delta(\textup{state}, x) := \textup{to} \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in (Q \mathbin{\backslash} {\textsc{DEAD}}) \f$
             * or \f$ x \not\in T^{-} \f$
             * or \f$ \textup{to} \not\in Q \f$
             */
            Builder& set_transition(StateIdx const& state, T const& x, StateIdx const& to) {
                check_ns_state(state);
                check_input(x);
                check_state(to);
                _transitions[x][state] = to;
                return *this;
            }

            /**
             * @brief Sets \f$ \delta(\textup{state}, \textsc{UNKNOWN}) := \textup{to} \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in (Q \mathbin{\backslash} {\textsc{DEAD}}) \f$
             * or \f$ \textup{to} \not\in Q \f$
             */
            Builder& set_unknown_transition(StateIdx const& state, StateIdx const& to) {
                check_ns_state(state);
                check_state(to);
                _unknown_transitions[state] = to;
                return *this;
            }

            /**
             * @brief Sets \f$ \delta(\textup{state}, x) := \textup{to} \quad \forall x \in (\Sigma \cup \textsc{UNKNOWN}) \f$.
             */
            Builder& set_all_transitions(StateIdx const& state, StateIdx const& to) {
                set_unknown_transition(state, to);
                for(auto& p: _transitions) {
                    p.second[state] = to;
                }
                return *this;
            }
            ///@}

            /**
             * @brief Swaps accepting and rejecting (i.e. non-accepting) states.
             *
             * Also changes all transitions into the dead state into transitions
             * into a "live state" (accepting and transitioning into itself for any input).
             *
             * @return This.
             */
            Builder& complement() {
                for(auto b: _accepting_states) {
                    b = !b;
                }

                StateIdx live = add_state().second;
                set_all_transitions(live, live);
                set_acceptance(live, true);

                auto cr = [live](std::optional<StateIdx>& opt){
                    if(opt.has_value() && opt.value() == DEAD_STATE) {
                        opt = live;
                    }
                };

                for(auto& p: _transitions) {
                    for_each(p.second, cr);
                }
                for_each(_unknown_transitions, cr);

                return *this;
            }

            /**
             * @brief Sets all missing transitions.
             * @return This.
             * @exception std::invalid_argument If \f$ \textup{to} \not\in Q \f$
             */
            Builder& complete(StateIdx const& to) {
                check_state(to);
                auto cr = [to](std::optional<StateIdx>& opt){
                    if(!opt.has_value()) {
                        opt = to;
                    }
                };

                for(auto& p: _transitions) {
                    for_each(p.second, cr);
                }
                for_each(_unknown_transitions, cr);

                return *this;
            }

            /**
             * @brief Tests whether  all transitions are set. 
             */
            bool is_complete() const {
                return std::all_of(
                        _transitions.cbegin(), _transitions.cend(),
                        [](auto p) {
                            return std::all_of(
                                p.second.cbegin(), p.second.cend(),
                                [](auto t){ return t.has_value(); }
                            );
                        }
                    ) &&
                    std::all_of(
                        _unknown_transitions.cbegin(), _unknown_transitions.cend(),
                        [](auto t){ return t.has_value(); }
                    );
            }

            /**
             * @name DFA finalization
             * @brief Builds the DFA.
             * @{
             */
            DFA finalize() const {
                if(state_count() == 0) {
                    throw std::invalid_argument("A DFA must have at least one state.");
                }
                if(!is_complete()) {
                    throw std::logic_error("Cannot finalize an incomplete DFA.");
                }

                return DFA(
                    transform_view(
                        views::all(_transitions),
                        [](auto p){ 
                            std::vector<StateIdx> transitions;
                            std::transform(
                                p.second.cbegin(), 
                                p.second.cend(), 
                                std::back_inserter(transitions), 
                                [](auto o){ return o.value(); }
                            );
                            return std::make_pair(p.first, transitions); 
                        }
                    ),
                    transform_view(
                        views::all(_unknown_transitions), 
                        [](auto o){ return o.value(); }
                    ),
                    _accepting_states
                );
            }

            operator DFA() const {
                return finalize();
            }
            ///@}

            /**
             * @name Make non-deterministic
             * @brief Transforms this builder into a builder for an equivalent DFA.
             *
             * Undefined transitions are left undefined.
             * @{
             */
            NFA<T>::Builder make_nondeterministic() const {
                typename NFA<T>::Builder builder(state_count());

                for(auto p: _transitions) {
                    builder.add_input(p.first);

                    for(StateIdx i = 0; i < state_count(); ++i) {
                        if(p.second[i].has_value() && p.second[i].value() != DEAD_STATE) {
                            builder.add_transition(i, p.first, p.second[i].value());
                        }
                    }
                }

                for(StateIdx i = 0; i < state_count(); ++i) {
                    if(_unknown_transitions[i].has_value() && _unknown_transitions[i].value() != DEAD_STATE) {
                        builder.add_unknown_transition(i, _unknown_transitions[i].value());
                    }
                }

                for(StateIdx i = 0; i < state_count(); ++i) {
                    builder.set_acceptance(i, _accepting_states[i]);
                }

                return builder;
            }

            operator NFA<T>::Builder() const {
                return make_nondeterministic();
            }
            ///@}
        };
    };


    /**
     * @brief <a href=https://en.wikipedia.org/wiki/Nondeterministic_finite_automaton>NFA-ε</a>.
     * 
     * This implementation uses a definition slightly different than the classic one.
     * A NFA-ε is defined as a 5-tuple consisting of:
     * - The states \f$ Q = [0,1,\ldots,n] \f$;
     * - The alphabet \f$ \Sigma = T^{-} \cup \textsc{UNKNOWN} \f$;
     * - The transition function \f$ \delta: Q \times (\Sigma \cup \varepsilon) \longmapsto \mathcal{P}(Q) \f$;
     * - The initial state \f$ q_0 = 0 \f$;
     * - The accepting states \f$ F \subset Q \f$.
     *
     * where \f$ T^{-} \subset T \f$.
     *
     * Even though \f$ \Sigma \not= T \f$, the automata works with any sequence
     * in \f$ T^{*} \f$ by mapping every value in \f$ x \in T \f$ to \f$ x \f$
     * if \f$ x \in T^{-} \f$ and to \f$ \textsc{UNKNOWN} \f$ otherwise.
     *
     * The differences with the standard definition are:
     * - The states are designed by (unsigned) integers: this isn't a huge limitation and makes it more efficient;
     * - The unknown transition and \f$ T^{-} \f$: allows to define the automaton on all values of type `T` without specifying every value;
     * - Default initial state: the initial state is always the state 0. Sorry but this was convenient.
     *
     * The NFA-ε defines a language 
     * \f$ \mathcal{L} = \left\{ w \in \Sigma^* \mid \Delta(\{0\} \times w) \in F \right\} \f$
     * where \f$ \Delta: \mathcal{P}(Q) \times \Sigma^* \longmapsto \mathcal{P}(Q) \f$ is the extended transition function
     * defined by
     * - \f$ \Delta(S \times \varepsilon) = S \f$;
     * - \f$ \Delta(S \times x \mathbin{::} w) = \Delta\left(\left(\bigcup\limits_{i \in S} \delta(s \times x) \right) \times w\right) \f$.
     *
     * Note that in order to be correct, the above definition requires some ε-closures which were 
     * omited for brevity.
     *
     * @tparam T Type of the alphabet.
     *
     * @warning This class could be reduced from NFA-ε into NFA for both simplicity and efficiency. 
     * The Builder would convert the NFA-ε into an NFA (i.e. the builder's interface would stay the same).
     */
    template<typename T>
    class NFA {
    public:
        /** 
         * @brief Type used to represent states. 
         * @hideinitializer
         */
        using StateIdx = std::size_t;
        /** 
         * @brief Type used to represent a set of states. 
         * @hideinitializer
         */
        using StateIndices = std::set<StateIdx>;

    private:
        std::unordered_map<T, std::vector<StateIndices>> _transitions;
        std::vector<StateIndices> _epsilon_transitions;
        std::vector<StateIndices> _unknown_transitions;
        std::vector<bool> _accepting_states;

        StateIdx const& check_state(StateIdx const& state) const {
            if(state >= state_count()) {
                throw std::invalid_argument("Invalid state: " + std::to_string(state));
            }
            return state;
        }

        StateIndices transition_unchecked(StateIdx const& state, T const& x) const {
            auto it =  _transitions.find(x);
            if(it != _transitions.cend()) {
                return it->second[state];
            }
            else {
                return _unknown_transitions[state];
            }
        }

        template<range Tr, range Et, range Ut, range As>
        NFA(Tr&& transitions, Et&& epsilons, Ut&& unknown_transitions, As&& accepting_states): 
        _transitions(std::ranges::cbegin(transitions), std::ranges::cend(transitions)), 
        _epsilon_transitions(std::ranges::cbegin(epsilons), std::ranges::cend(epsilons)),
        _unknown_transitions(std::ranges::cbegin(unknown_transitions), std::ranges::cend(unknown_transitions)), 
        _accepting_states(std::ranges::cbegin(accepting_states), std::ranges::cend(accepting_states)) {
            auto s = _unknown_transitions.size();
            auto check = [&s](auto& ls, std::string const& name){ 
                if(ls.size() != s) {
                    throw std::invalid_argument("Table size mismatch: " + name + '.');
                }
            };

            check(_accepting_states, "accepting states");
            check(_epsilon_transitions, "epsilon");
            for(auto p: _transitions) {
                check(p.second, Stringify<T>::convert(p.first));
            }
        }

        void epsilon_closure(StateIndices& current) const {
            std::queue<StateIdx> queue;
            for(auto c : current) {
                queue.push(c);
            }

            while(!queue.empty()) {
                for(StateIdx state: _epsilon_transitions[queue.front()]) {
                    if(current.insert(state).second) {
                        queue.push(state);
                    }
                }

                queue.pop();
            }
        }

    public:
        /**
         * @brief Returns the number of states.
         */
        StateIdx state_count() const {
            return _unknown_transitions.size();
        }

        /**
         * @brief Tests whether \f$ \textup{state} \in F\f$.
         * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
         */
        bool is_accepting(StateIdx const& state) const {
            return _accepting_states[check_state(state)];
        }

        /**
         * @brief Returns \f$ \delta(\textup{state} \times x) \f$.
         *
         * @exception std::invalid_argument If \f$ x \not\in Q \f$
         * or \f$ x \not\in T^{-} \f$.
         */
        StateIndices transition(StateIdx const& state, T const& x) const {
            check_state(state);

            auto it =  _transitions.find(x);
            if(it != _transitions.cend()) {
                return it->second[state];
            }
            else {
                throw std::invalid_argument("Invalid input: " + Stringify<T>::convert(x));
            }
        }

        /**
         * @brief Returns \f$ \delta(\textup{state} \times \varepsilon) \f$.
         * @exception std::invalid_argument If \f$ x \not\in Q \f$
         */
        StateIndices epsilon_transition(StateIdx const& current) const {
            return _epsilon_transitions[check_state(current)];
        }

        /**
         * @brief Returns \f$ \delta(\textup{state} \times \textsc{UNKNOWN}) \f$.
         * @exception std::invalid_argument If \f$ x \not\in Q \f$
         */
        StateIndices unknown_transition(StateIdx const& current) const {
            return _unknown_transitions[check_state(current)];
        }

        /**
         * @brief Returns \f$ T^{-} \f$.
         */
        auto alphabet() const {
            return transform_view(_transitions, [](auto p){ return p.first; });
        }

        /**
         * @brief Tests whether the NFA has ε-transitions.
         */
        bool has_epsilon_transitions() const {
            return any_of(_epsilon_transitions, [](auto s){ return !s.empty(); });
        }

        /**
         * @brief Tests whether \f$ \textup{sequence} \in \mathcal{L} \f$
         * 
         * @tparam R Type of the input sequence.
         * @param sequence The sequence to test for language-membership.
         */
        template<range_of<T> R>
        bool accepts(R&& sequence) const {
            StateIndices current;
            current.insert(0);

            epsilon_closure(current);

            for(
                auto beg = std::ranges::cbegin(sequence), end = std::ranges::cend(sequence); 
                beg != end; 
                ++beg
            ) {
                StateIndices next;

                for(StateIdx state: current) {
                    for(StateIdx n: transition_unchecked(state, *beg)) {
                        next.insert(n);
                    }
                }

                current = next;
                epsilon_closure(current);
            }

            return any_of(current, [this](auto s){ return this->is_accepting(s); });
        }

        /**
         * @brief Tests whether \f$ \textup{sequence} \in \mathcal{L} \f$
         * @see \ref accepts<R>()
         */
        bool accepts(std::initializer_list<T> sequence) const {
            return accepts(views::all(sequence));
        }


        /**
         * @brief Allows NFA creation.
         */
        class Builder final {
            using OptStateIdx = std::optional<StateIdx>;

            std::unordered_map<T, std::vector<StateIndices>> _transitions;
            std::vector<StateIndices> _epsilon_transitions;
            std::vector<StateIndices> _unknown_transitions;
            std::vector<bool> _accepting_states;

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
                }
                return state;
            }

            template<range_of<StateIdx> R>
            R const& check_states(R const& states) const {
                for(auto s: states) {
                    check_state(s);
                }
                return states;
            }

            T const& check_input(T const& input) const {
                if(!_transitions.contains(input)) {
                    throw std::invalid_argument("Invalid input: " + Stringify<T>::convert(input));
                }
                return input;
            }

            std::vector<StateIndices> const& transitions(T const& value) const {
                auto it =  _transitions.find(value);
                if(it != _transitions.cend()) {
                    return it->second;
                }
                else {
                    return _unknown_transitions;
                }
            }

        public:
            /**
             * @brief Creates a builder with initial \f$ T^{-} = \textup{inputs}\f$ and `size` states.
             */
            template<range_of<T> R>
            Builder(R&& inputs, StateIdx size = 0): 
            _transitions(), 
            _epsilon_transitions(size, StateIndices{}), 
            _unknown_transitions(size, StateIndices{}), 
            _accepting_states(size, 0) 
            {
                for(auto input: inputs) {
                    add_input(input);
                }
            }

            /**
             * @brief Creates a builder with initial \f$ T^{-} = \textup{inputs}\f$ and `size` states.
             */
            Builder(std::initializer_list<T> inputs = {}, StateIdx size = 0): Builder(views::all(inputs), size) {}
            
            /**
             * @brief Creates a builder with `size` states.
             */
            Builder(StateIdx size): Builder(views::empty<T>, size) {}

            /**
             * @brief Returns the number of states.
             */
            StateIdx state_count() const {
                return _unknown_transitions.size();
            }

            /**
             * @brief Tests whether \f$ \textup{state} \in F\f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             */
            bool is_accepting(StateIdx const& state) const {
                return _accepting_states[check_state(state)];
            }

            /**
             * @name Transition getters 
             * @{
             */
            /**
             * @brief Returns \f$ \delta(\textup{state} \times x) \f$.
             *
             * If \f$ x \not\in T^{-} \f$, then this is equivalent to \ref unknown_transition().
             *
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$.
            */
            StateIndices transition(StateIdx const& state, T const& x) const {
                check_state(state);

                auto it =  _transitions.find(x);
                if(it != _transitions.cend()) {
                    return it->second[state];
                }
                else {
                    return _unknown_transitions[state];
                }
            }

            /**
             * @brief Returns \f$ \delta(\textup{state} \times \varepsilon) \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             */
            std::optional<StateIdx> epsilon_transition(StateIdx const& state) const {
                return _epsilon_transitions[check_state(state)];
            }

            /**
             * @brief Returns \f$ \delta(\textup{state} \times \textsc{UNKNOWN}) \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             */
            std::optional<StateIdx> unknown_transition(StateIdx const& state) const {
                return _unknown_transitions[check_state(state)];
            }
            ///@}
            
            /**
             * @brief Returns \f$ T^{-} \f$.
             */
            auto alphabet() const {
                return transform_view(_transitions, [](auto p){ return p.first; });
            }

            /**
             * @brief Adds \f$ t \f$ to \f$ T^{-} \f$.
             * @return This.
             */
            Builder& add_input(T const& input) {
                _transitions.insert(std::pair{
                    input, 
                    _unknown_transitions
                });
                return *this;
            }

            /**
             * @name State addition
             * @brief Adds a new state \f$ i \f$.
             * 
             * @param to The initial value of \f$ \delta(i, x) \forall x \in (\Sigma \cup \textsc{UNKNOWN}) \f$.
             * @param accepting Whether \f$ i \in F \f$.
             * @return  This and the new state's index, \f$ i \f$.
             * @{
             */
            /**
             * @exception std::invalid_argument If \f$ \textup{to} \not\subset Q \f$
             */
            std::pair<Builder&, StateIdx> add_state(StateIndices const& to = {}, bool accepting = false) {
                check_states(to);

                for(auto p: _transitions) {
                    p.second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _epsilon_transitions.emplace_back();
                _accepting_states.push_back(accepting);
                return { *this, state_count()-1 };
            }

            /**
             * @exception std::invalid_argument If \f$ \textup{to} \not\in Q \f$
             */
            std::pair<Builder&, StateIdx> add_state(StateIdx const& to, bool accepting = false) {
                return add_state(StateIndices{to}, accepting);
            }

            std::pair<Builder&, StateIdx> add_state(bool accepting) {
                return add_state(StateIndices{}, accepting);
            }
            ///@}

            /**
             * @name Acceptance setters
             * @return This.
             * @{
             */
            /**
             * @brief Sets whether \f$ \textup{state} \in F \f$.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             */
            Builder& set_acceptance(StateIdx const& state, bool value) {
                check_state(state);
                _accepting_states[state] = value;
                return *this;
            }

            /**
             * @brief Sets whether \f$ \textup{states} \subset F \f$.
             * @exception std::invalid_argument If \f$ \textup{states} \not\subset Q \f$
             */
            template<range_of<StateIdx> R>
            Builder& set_acceptance(R&& states, bool value) {
                for(auto state: states) {
                    check_state(state);
                    _accepting_states[state] = value;
                }
                return *this;
            }

            /**
             * @brief Sets whether \f$ \textup{states} \subset F \f$.
             * @exception std::invalid_argument If \f$ \textup{states} \not\subset Q \f$
             */
            Builder& set_acceptance(std::initializer_list<StateIdx> states, bool value) {
                return set_acceptance(views::all(states), value);
            }
            ///@}

            /**
             * @name Transition addition
             * @brief Adds \f$ \textup{to} \f$ to \f$ \delta(\textup{state}, x) \f$.
             * @return This.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             * or \f$ x \not\in T^{-} \f$
             * or \f$ \textup{to} \not\subset Q \f$
             * @{
             */
            Builder& add_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                check_state(state);
                check_input(input);
                check_state(to);
                _transitions[input][state].insert(to);
                return *this;
            }

            template<range_of<StateIdx> R>
            Builder& add_transitions(StateIdx const& state, T const& input, R&& to) {
                check_state(state);
                check_input(input);
                check_states(to);
                _transitions[input][state].insert(std::ranges::cbegin(to), std::ranges::cend(to));
                return *this;
            }

            Builder& add_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_transitions(state, views::all(to));
            }
            ///@}

            /**
             * @name ε-transition addition
             * @brief Adds \f$ \textup{to} \f$ to \f$ \delta(\textup{state}, \varepsilon) \f$.
             * @return This.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             * or \f$ \textup{to} \not\subset Q \f$
             * @{
             */
            Builder& add_epsilon_transition(StateIdx const& state, StateIdx to) {
                check_state(state);
                check_state(to);
                _epsilon_transitions[state].insert(to);
                return *this;
            }

            template<range_of<StateIdx> R>
            Builder& add_epsilon_transitions(StateIdx const& state, R&& to) {
                check_state(state);
                check_states(to);
                _epsilon_transitions[state].insert(std::ranges::cbegin(to), std::ranges::cend(to));
                return *this;
            }

            Builder& add_epsilon_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_epsilon_transitions(state, views::all(to));
            }
            ///@}

            /**
             * @name UNKNOWN-transition addition
             * @brief Adds \f$ \textup{to} \f$ to \f$ \delta(\textup{state}, \textsc{UNKNOWN}) \f$.
             * @return This.
             * @exception std::invalid_argument If \f$ \textup{state} \not\in Q \f$
             * or \f$ \textup{to} \not\subset Q \f$
             * @{
             */
            Builder& add_unknown_transition(StateIdx const& state, StateIdx to) {
                check_state(state);
                check_state(to);
                _unknown_transitions[state].insert(to);
                return *this;
            }

            template<range_of<StateIdx> R>
            Builder& add_unknown_transitions(StateIdx const& state, R&& to) {
                check_state(state);
                check_states(to);
                _unknown_transitions[state].insert(std::ranges::cbegin(to), std::ranges::cend(to));
                return *this;
            }

            Builder& add_unknown_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_unknown_transitions(state, views::all(to));
            }
            ///@}

            /**
             * @brief Returns all states reachable from `state` using only ε-transitions.
             */
            StateIndices epsilon_closure(StateIdx state) const {
                std::queue<StateIdx> queue;
                queue.push(state);

                StateIndices closure;

                while(!queue.empty()) {
                    for(StateIdx state: _epsilon_transitions[queue.front()]) {
                        if(closure.insert(state).second) {
                            queue.push(state);
                        }
                    }

                    queue.pop();
                }

                return closure;
            }

            /**
             * @brief Removes all ε-transitions without changing the language \f$ \mathcal{L} \f$.
             */
            Builder& epsilon_elimination() {
                for(StateIdx i = 0; i < state_count(); ++i) {
                    for(StateIdx j: epsilon_closure(i)) {

                        for(auto& p: _transitions) {
                            add_transitions(i, p.first, p.second[j]);
                        }

                        add_unknown_transitions(i, _unknown_transitions[j]);

                        _epsilon_transitions[i].clear();

                        if(_accepting_states[j]) {
                            _accepting_states[i] = true;
                        }
                    }
                }
                return *this;
            }

            /**
             * @brief Integrate another NFA into this one.
             * @return This and the the new index of `that` initial state.
             */
            std::pair<Builder&, StateIdx> meld(Builder const& that) {
                for(auto input: that.alphabet()) {
                    add_input(input);
                }

                StateIdx offset = state_count();
                auto tr = [offset](std::set<StateIdx> indices){ 
                    std::set<StateIdx> result;
                    transform(indices, std::inserter(result, result.end()), [offset](StateIdx idx){ return idx + offset; }); 
                    return result;
                };
                
                for(auto& p: _transitions) {
                    transform(that.transitions(p.first), std::back_inserter(p.second), tr);
                }

                transform(that._unknown_transitions, std::back_inserter(_unknown_transitions), tr);
                transform(that._epsilon_transitions, std::back_inserter(_epsilon_transitions), tr);
                copy(that._accepting_states, std::back_inserter(_accepting_states));

                return { *this, offset };
            }

            /**
             * @name NFA finalization
             * @brief Builds the NFA.
             * @{
             */
            NFA finalize() const {
                if(state_count() == 0) {
                    throw std::invalid_argument("A NFA must have at least one state.");
                }

                return NFA(
                    _transitions,
                    _epsilon_transitions,
                    _unknown_transitions,
                    _accepting_states
                );
            }

            operator NFA() const {
                return finalize();
            }
            ///@}

            /**
             * @name Make deterministic
             * @brief Transforms this builder into a builder for an equivalent NFA.
             *
             * Undefined transitions are set to the dead state.
             * @{
             */
            DFA<T>::Builder make_deterministic() {
                epsilon_elimination();

                auto inputs = transform_view(_transitions, [](auto p){ return p.first; });
                auto transition = [this](std::vector<bool> state, T const& input) {
                    std::vector<bool> out(state_count(), false);

                    for(StateIdx i = 0; i < state_count(); ++i) {
                        if(state[i]) {
                            for(StateIdx j: _transitions.at(input)[i]) {
                                out[j] = true;
                            }
                        }
                    }

                    return out;
                };

                auto u_transition = [this](std::vector<bool> state) {
                    std::vector<bool> out(state_count(), false);

                    for(StateIdx i = 0; i < state_count(); ++i) {
                        if(state[i]) {
                            for(StateIdx j: _unknown_transitions[i]) {
                                out[j] = true;
                            }
                        }
                    }

                    return out;
                };

                auto accepting = [this](std::vector<bool> state) {
                    for(StateIdx i = 0; i < state_count(); ++i) {
                        if(state[i] && this->is_accepting(i)) {
                            return true;
                        }
                    }
                    return false;
                };
                
                std::vector<bool> dead(state_count(), false);
                std::vector<bool> start(dead);
                start[0] = true;

                typename DFA<T>::Builder builder(inputs);
                std::unordered_map<std::vector<bool>, typename DFA<T>::StateIdx> indices;
                indices.emplace(start, builder.add_state().second);
                indices.emplace(dead, DFA<T>::DEAD_STATE);

                std::queue<std::vector<bool>> queue;
                queue.push(start);

                while(!queue.empty()) {
                    std::vector<bool> current = queue.front();
                    queue.pop();

                    for(auto input: inputs) {
                        std::vector<bool> to = transition(current, input);

                        if(!indices.contains(to)) {
                            indices.emplace(to, builder.add_state().second);
                            queue.push(to);
                        }

                        builder.set_transition(indices.at(current), input, indices.at(to));
                    }

                    {
                        std::vector<bool> to = u_transition(current);

                        if(!indices.contains(to)) {
                            indices.emplace(to, builder.add_state().second);
                            queue.push(to);
                        }

                        builder.set_unknown_transition(indices.at(current), indices.at(to));
                    }
                }

                for(auto p: indices) {
                    if(accepting(p.first)) {
                        builder.set_acceptance(p.second, true);
                    }
                }

                builder.complete(indices[dead]);

                return builder;
            }

            DFA<T>::Builder make_deterministic() const {
                return Builder(*this).make_deterministic();
            }

            operator DFA<T>::Builder() const {
                return make_deterministic();
            } 
            ///@}
        };

    };
}