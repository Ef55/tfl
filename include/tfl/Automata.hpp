#pragma once

#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <optional>
#include <algorithm>
#include <set>
#include <queue>

#include "tfl/Stringify.hpp"

namespace tfl {

    namespace {
        using namespace std::ranges;
    }

    template<typename T>
    class DFA final {
    public:
        using StateIdx = std::size_t;

    private:
        std::unordered_map<T, std::vector<StateIdx>> _transitions;
        std::vector<StateIdx> _unknown_transitions;
        std::vector<bool> _accepting_states;

        StateIdx const& check_state(StateIdx const& state) const {
            if(state >= state_count()) {
                throw std::invalid_argument("Impossible current state");
            }
            return state;
        }

        StateIdx transition(StateIdx const& current, T const& value) const {
            check_state(current);
            return transitions(value)[current];
        }

        template<range Tr, range Ut, range As>
        DFA(Tr&& transitions, Ut&& unknown_transitions, As&& accepting_states): 
        _transitions(transitions.begin(), transitions.end()), 
        _unknown_transitions(unknown_transitions.begin(), unknown_transitions.end()), 
        _accepting_states(accepting_states.begin(), accepting_states.end()) {
            
            auto s = _unknown_transitions.size();
            auto check = [&s](auto& ls, std::string const& name){ 
                if(ls.size() != s) {
                    throw std::invalid_argument("Table size mismatch: " + name + '.');
                }
            };

            check(_accepting_states, "accepting states");
            for(auto p: _transitions) {
                check(p.second, Stringify<T>{}(p.first));
            }
        }

    public:
        StateIdx state_count() const {
            return _unknown_transitions.size();
        }

        std::vector<StateIdx> const& transitions(T const& value) const {
            auto it =  _transitions.find(value);
            if(it != _transitions.cend()) {
                return it->second;
            }
            else {
                return _unknown_transitions;
            }
        }

        std::vector<bool> const& accepting_states() const {
            return _accepting_states;
        }

        template<range R>
        bool accepts(R&& range) const {
            StateIdx state = 0;
            for(
                auto beg = range.begin(), end = range.end(); 
                beg != end; 
                ++beg
            ) {
                state = transition(state, *beg);
            }

            return accepting_states()[state];
        }

        bool accepts(std::initializer_list<T> ls) const {
            return accepts<std::initializer_list<T>&>(ls);
        }

        class Builder final {
            using OptStateIdx = std::optional<StateIdx>;

            std::unordered_map<T, std::vector<OptStateIdx>> _transitions;
            std::vector<OptStateIdx> _unknown_transitions;
            std::vector<bool> _accepting_states;

            void sanity() const {
                auto s = state_count();
                auto check = [&s](auto& ls, std::string const& name){ 
                    if(ls.size() != s) {
                        throw std::invalid_argument("Table size mismatch: " + name + '.');
                    }
                };

                for(auto p: _transitions) {
                    check(p.second, Stringify<T>{}(p.first));
                }
                check(_unknown_transitions, "unknown transitions");
                check(_accepting_states, "accepting states");
            }

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
                }
                return state;
            }

            T const& check_input(T const& input) const {
                if(!_transitions.contains(input)) {
                    throw std::invalid_argument("Invalid input: " + Stringify<T>{}(input));
                }
                return input;
            }

        public:
            template<range R>
            Builder(R&& inputs, StateIdx size = 1): _transitions(), _unknown_transitions(size, std::nullopt), _accepting_states(size, 0) {
                if(size == 0) {
                    throw std::invalid_argument("A DFA must have at least one state.");
                }
                for(auto& input: inputs) {
                    add_input(input);
                }
            }
            Builder(std::initializer_list<T> states = {}, StateIdx size = 0): Builder(views::all(states), size) {}
            Builder(StateIdx size = 0): Builder(views::empty<T>, size) {}

            StateIdx state_count() const {
                return _unknown_transitions.size();
            }

            std::vector<std::optional<StateIdx>> const& transitions(T const& value) const {
                sanity();
                auto it =  _transitions.find(value);
                if(it != _transitions.cend()) {
                    return it->second;
                }
                else {
                    return _unknown_transitions;
                }
            }

            std::vector<bool> const& accepting_states() const {
                sanity();
                return _accepting_states;
            }

            Builder& add_input(T const& input) {
                sanity();
                _transitions.insert(std::pair{
                    input, 
                    _unknown_transitions
                });
                sanity();
                return *this;
            }

            std::pair<Builder&, StateIdx> add_state(std::optional<StateIdx> const& to = std::nullopt, bool accepting = false) {
                sanity();
                for(auto& it: _transitions) {
                    it->second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _accepting_states.push_back(accepting);
                sanity();
                return { *this, state_count()-1 };
            }

            Builder& set_acceptance(StateIdx const& state, bool value) {
                sanity();
                check_state(state);
                _accepting_states[state] = value;
                sanity();
                return *this;
            }

            template<range R>
            Builder& set_acceptance(R&& states, bool value) {
                sanity();
                for(auto state: states) {
                    check_state(state);
                    _accepting_states[state] = value;
                }
                sanity();
                return *this;
            }

            Builder& set_acceptance(std::initializer_list<T> states, bool value) {
                return set_acceptance<std::initializer_list<T>&>(states, value);
            }

            Builder& set_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                sanity();
                check_state(state);
                check_input(input);
                _transitions[input][state] = to;
                sanity();
                return *this;
            }

            template<range R>
            Builder& set_transitions(StateIdx const& state, R&& transitions) {
                for(auto& p: transitions) {
                    set_transition(state, std::get<0>(p), std::get<1>(p));
                }
                return *this;
            }

            Builder& set_transitions(StateIdx const& state, std::initializer_list<std::pair<T, StateIdx>> transitions) {
                return set_transitions<std::initializer_list<std::pair<T, StateIdx>>&>(state, transitions);
            }

            Builder& set_unknown_transition(StateIdx const& state, StateIdx const& to) {
                sanity();
                check_state(state);
                _unknown_transitions[state] = to;
                sanity();
                return *this;
            }

            Builder& set_all_transitions(StateIdx const& state, StateIdx const& to) {
                set_unknown_transition(state, to);
                for(auto& p: _transitions) {
                    p.second[state] = to;
                }
                return *this;
            }

            bool is_complete() const {
                sanity();
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

            DFA finalize() const {
                if(!is_complete()) {
                    throw std::logic_error("Cannot finalize and incomplete DFA.");
                }

                return DFA(
                    transform_view(
                        subrange(_transitions.cbegin(), _transitions.cend()),
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
                        subrange(_unknown_transitions.cbegin(), _unknown_transitions.cend()), 
                        [](auto o){ return o.value(); }
                    ),
                    _accepting_states
                );
            }

            operator DFA() const {
                return finalize();
            }
        };
    };



    template<typename T>
    class NFA {
    public:
        using StateIdx = std::size_t;
        using StateIndices = std::set<StateIdx>;

    private:
        std::unordered_map<T, std::vector<StateIndices>> _transitions;
        std::vector<StateIndices> _epsilon_transitions;
        std::vector<StateIndices> _unknown_transitions;
        std::vector<bool> _accepting_states;

        template<range Tr, range Et, range Ut, range As>
        NFA(Tr&& transitions, Et&& epsilons, Ut&& unknown_transitions, As&& accepting_states): 
        _transitions(transitions.begin(), transitions.end()), 
        _epsilon_transitions(epsilons.begin(), epsilons.end()),
        _unknown_transitions(unknown_transitions.begin(), unknown_transitions.end()), 
        _accepting_states(accepting_states.begin(), accepting_states.end()) {
            auto s = _unknown_transitions.size();
            auto check = [&s](auto& ls, std::string const& name){ 
                if(ls.size() != s) {
                    throw std::invalid_argument("Table size mismatch: " + name + '.');
                }
            };

            check(_accepting_states, "accepting states");
            check(_epsilon_transitions, "epsilon");
            for(auto p: _transitions) {
                check(p.second, Stringify<T>{}(p.first));
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
        StateIdx state_count() const {
            return _unknown_transitions.size();
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

        std::vector<bool> const& accepting_states() const {
            return _accepting_states;
        }

        bool has_epsilon_transitions() const {
            return any_of(_epsilon_transitions, [](auto s){ return !s.empty(); });
        }

        template<range R>
        bool accepts(R&& range) const {
            StateIndices current;
            current.insert(0);

            epsilon_closure(current);

            for(
                auto beg = range.begin(), end = range.end(); 
                beg != end; 
                ++beg
            ) {
                StateIndices next;

                for(StateIdx state: current) {
                    for(StateIdx n: transitions(*beg)[state]) {
                        next.insert(n);
                    }
                }

                current = next;
                epsilon_closure(current);
            }

            return any_of(current, [this](auto s){ return this->accepting_states()[s]; });
        }

        bool accepts(std::initializer_list<T> ls) const {
            return accepts<std::initializer_list<T>&>(ls);
        }

        class Builder final {
            using OptStateIdx = std::optional<StateIdx>;

            std::unordered_map<T, std::vector<StateIndices>> _transitions;
            std::vector<StateIndices> _epsilon_transitions;
            std::vector<StateIndices> _unknown_transitions;
            std::vector<bool> _accepting_states;

            void sanity() const {
                auto s = state_count();
                auto check = [&s](auto& ls, std::string const& name){ 
                    if(ls.size() != s) {
                        throw std::invalid_argument("Table size mismatch: " + name + '.');
                    }
                };

                for(auto p: _transitions) {
                    check(p.second, Stringify<T>{}(p.first));
                }
                check(_epsilon_transitions, "epsilon transitions");
                check(_unknown_transitions, "unknown transitions");
                check(_accepting_states, "accepting states");
            }

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
                }
                return state;
            }

            T const& check_input(T const& input) const {
                if(!_transitions.contains(input)) {
                    throw std::invalid_argument("Invalid input: " + Stringify<T>{}(input));
                }
                return input;
            }

        public:
            template<range R>
            Builder(R&& inputs, StateIdx size = 1): 
            _transitions(), 
            _epsilon_transitions(size, StateIndices{}), 
            _unknown_transitions(size, StateIndices{}), 
            _accepting_states(size, 0) 
            {
                if(size == 0) {
                    throw std::invalid_argument("A DFA must have at least one state.");
                }
                for(auto& input: inputs) {
                    add_input(input);
                }
            }
            Builder(std::initializer_list<T> states = {}, StateIdx size = 0): Builder(views::all(states), size) {}
            Builder(StateIdx size = 0): Builder(views::empty<T>, size) {}

            StateIdx state_count() const {
                return _unknown_transitions.size();
            }

            std::vector<StateIndices> const& transitions(T const& value) const {
                sanity();
                auto it =  _transitions.find(value);
                if(it != _transitions.cend()) {
                    return it->second;
                }
                else {
                    return _unknown_transitions;
                }
            }

            std::vector<bool> const& accepting_states() const {
                sanity();
                return _accepting_states;
            }

            StateIndices epsilon_closure(StateIdx state) const {
                std::queue<StateIdx> queue;
                queue.push(state);

                // TODO refactor to reduce memory usage
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

            Builder& add_input(T const& input) {
                sanity();
                _transitions.insert(std::pair{
                    input, 
                    _unknown_transitions
                });
                sanity();
                return *this;
            }

            std::pair<Builder&, StateIdx> add_state(StateIndices const& to = {}, bool accepting = false) {
                sanity();
                for(auto& it: _transitions) {
                    it->second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _accepting_states.push_back(accepting);
                sanity();
                return { *this, state_count()-1 };
            }

            std::pair<Builder&, StateIdx> add_state(StateIdx const& to, bool accepting = false) {
                return add_state({to}, accepting);
            }

            Builder& set_acceptance(StateIdx const& state, bool value) {
                sanity();
                check_state(state);
                _accepting_states[state] = value;
                sanity();
                return *this;
            }

            template<range R>
            Builder& set_acceptance(R&& states, bool value) {
                sanity();
                for(auto state: states) {
                    check_state(state);
                    _accepting_states[state] = value;
                }
                sanity();
                return *this;
            }

            Builder& set_acceptance(std::initializer_list<T> states, bool value) {
                return set_acceptance<std::initializer_list<T>&>(states, value);
            }

            Builder& add_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                sanity();
                check_state(state);
                check_input(input);
                _transitions[input][state].insert(to);
                sanity();
                return *this;
            }

            template<range R>
            Builder& add_transitions(StateIdx const& state, T const& input, R&& to) {
                sanity();
                check_state(state);
                check_input(input);
                _transitions[input][state].insert(to.begin(), to.end());
                sanity();
                return *this;
            }

            template<range R>
            Builder& add_epsilon_transitions(StateIdx const& state, R&& to) {
                sanity();
                check_state(state);
                _epsilon_transitions[state].insert(to.begin(), to.end());
                sanity();
                return *this;
            }

            Builder& add_epsilon_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_epsilon_transitions<std::initializer_list<StateIdx>&>(state, to);
            }

            Builder& add_epsilon_transition(StateIdx const& state, StateIdx to) {
                sanity();
                check_state(state);
                _epsilon_transitions[state].insert(to);
                sanity();
                return *this;
            }

            template<range R>
            Builder& add_unknown_transitions(StateIdx const& state, R&& to) {
                sanity();
                check_state(state);
                _unknown_transitions[state].insert(to.begin(), to.end());
                sanity();
                return *this;
            }

            Builder& add_unknown_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_unknown_transitions<std::initializer_list<StateIdx>&>(state, to);
            }

            Builder& add_unknown_transition(StateIdx const& state, StateIdx to) {
                sanity();
                check_state(state);
                _unknown_transitions[state].insert(to);
                sanity();
                return *this;
            }

            Builder& epsilon_elimination() {
                sanity();
                for(StateIdx i = 0; i < state_count(); ++i) {
                    for(StateIdx j: epsilon_closure(i)) {

                        for(auto& p: _transitions) {
                            add_transitions(i, p.first, p.second[j]);
                        }

                        add_unknown_transitions(i, _unknown_transitions[j]);

                        _epsilon_transitions[i].clear();
                    }
                }
                sanity();
                return *this;
            }

            NFA finalize() const {
                sanity();

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
        };

    };
}