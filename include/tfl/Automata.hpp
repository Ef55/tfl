#pragma once

#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <optional>
#include <algorithm>

#include "tfl/Stringify.hpp"

namespace tfl {

    namespace {
        using namespace std::ranges;
    }

    template<typename T>
    class DFA final {
        using StateIdx = std::size_t;

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

        template<class It>
        bool accepts(It beg, It end) const {
            StateIdx state = 0;
            for(; beg != end; ++beg) {
                state = transition(state, *beg);
            }

            return accepting_states()[state];
        }

        template<range R>
        bool accepts(R&& range) const {
            return accepts(range.begin(), range.end());
        }

        bool accepts(std::initializer_list<T> ls) const {
            return accepts(ls.begin(), ls.end());
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
            Builder(StateIdx size = 0): _transitions(), _unknown_transitions(size, std::nullopt), _accepting_states(size, 0) {}

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

            StateIdx add_state(bool accepting = false) {
                sanity();
                for(auto& it: _transitions) {
                    it->second.emplace_back(std::nullopt);
                }
                _unknown_transitions.emplace_back(std::nullopt);
                _accepting_states.push_back(accepting);
                sanity();
                return state_count()-1;
            }

            bool add_input(T const& input) {
                sanity();
                bool res = _transitions.insert(std::pair{
                        input, 
                        std::vector(state_count(), std::optional<StateIdx>(std::nullopt))
                    }).second;
                sanity();
                return res;
            }

            StateIdx add_state(StateIdx const& to, bool accepting = false) {
                sanity();
                for(auto& it: _transitions) {
                    it->second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _accepting_states.push_back(accepting);
                sanity();
                return state_count()-1;
            }

            void set_acceptance(StateIdx const& state, bool value) {
                sanity();
                check_state(state);
                _accepting_states[state] = value;
                sanity();
            }

            void set_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                sanity();
                check_state(state);
                check_input(input);
                _transitions[input][state] = to;
                sanity();
            }

            void set_unknown_transition(StateIdx const& state, StateIdx const& to) {
                sanity();
                check_state(state);
                _unknown_transitions[state] = to;
                sanity();
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

}