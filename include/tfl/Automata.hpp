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
    class NFA;

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
                check(p.second, Stringify<T>::convert(p.first));
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

        std::pair<StateIdx, bool> step(StateIdx start, T const& value) const {
            StateIdx end = transition(start, value);
            return {end, accepting_states()[end]};
        }

        class Builder final {
            using OptStateIdx = std::optional<StateIdx>;

            std::unordered_map<T, std::vector<OptStateIdx>> _transitions;
            std::vector<OptStateIdx> _unknown_transitions;
            std::vector<bool> _accepting_states;

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
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
            template<range R>
            Builder(R&& inputs, StateIdx size = 0): _transitions(), _unknown_transitions(size, std::nullopt), _accepting_states(size, 0) {
                for(auto input: inputs) {
                    add_input(input);
                }
            }
            Builder(std::initializer_list<T> states = {}, StateIdx size = 0): Builder(views::all(states), size) {}
            Builder(StateIdx size): Builder(views::empty<T>, size) {}

            StateIdx state_count() const {
                return _unknown_transitions.size();
            }

            std::vector<std::optional<StateIdx>> const& transitions(T const& value) const {
                auto it =  _transitions.find(value);
                if(it != _transitions.cend()) {
                    return it->second;
                }
                else {
                    return _unknown_transitions;
                }
            }

            std::vector<std::optional<StateIdx>> const& unknown_transitions() const {
                return _unknown_transitions;
            }

            std::vector<bool> const& accepting_states() const {
                return _accepting_states;
            }

            auto inputs() const {
                return transform_view(_transitions, [](auto p){ return p.first; });
            }

            Builder& add_input(T const& input) {
                _transitions.insert(std::pair{
                    input, 
                    _unknown_transitions
                });
                return *this;
            }

            std::pair<Builder&, StateIdx> add_state(std::optional<StateIdx> const& to = std::nullopt, bool accepting = false) {
                for(auto& it: _transitions) {
                    it.second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _accepting_states.push_back(accepting);
                return { *this, state_count()-1 };
            }

            Builder& set_acceptance(StateIdx const& state, bool value) {
                check_state(state);
                _accepting_states[state] = value;
                return *this;
            }

            Builder& complement() {
                for(auto b: _accepting_states) {
                    b = !b;
                }
                return *this;
            }

            template<range R>
            Builder& set_acceptance(R&& states, bool value) {
                for(auto state: states) {
                    check_state(state);
                    _accepting_states[state] = value;
                }
                return *this;
            }

            Builder& set_acceptance(std::initializer_list<T> states, bool value) {
                return set_acceptance<std::initializer_list<T>&>(states, value);
            }

            Builder& set_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                check_state(state);
                check_input(input);
                _transitions[input][state] = to;
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
                check_state(state);
                _unknown_transitions[state] = to;
                return *this;
            }

            Builder& set_all_transitions(StateIdx const& state, StateIdx const& to) {
                set_unknown_transition(state, to);
                for(auto& p: _transitions) {
                    p.second[state] = to;
                }
                return *this;
            }

            Builder& complete(StateIdx const& to) {
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

            DFA finalize() const {
                if(state_count() == 0) {
                    throw std::invalid_argument("A DFA must have at least one state.");
                }
                if(!is_complete()) {
                    throw std::logic_error("Cannot finalize an incomplete DFA.");
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

            NFA<T>::Builder make_nondeterministic() const {
                typename NFA<T>::Builder builder(state_count());

                for(auto p: _transitions) {
                    builder.add_input(p.first);

                    for(StateIdx i = 0; i < state_count(); ++i) {
                        if(p.second[i].has_value()) {
                            builder.add_transition(i, p.first, p.second[i].value());
                        }
                    }
                }

                for(StateIdx i = 0; i < state_count(); ++i) {
                    if(_unknown_transitions[i].has_value()) {
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

            StateIdx const& check_state(StateIdx const& state) const {
                if(state >= state_count()) {
                    throw std::invalid_argument("Invalid state: " + std::to_string(state));
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
            template<range R>
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
            Builder(std::initializer_list<T> states = {}, StateIdx size = 0): Builder(views::all(states), size) {}
            Builder(StateIdx size): Builder(views::empty<T>, size) {}

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

            auto inputs() const {
                return transform_view(_transitions, [](auto p){ return p.first; });
            }

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

            Builder& add_input(T const& input) {
                _transitions.insert(std::pair{
                    input, 
                    _unknown_transitions
                });
                return *this;
            }

            std::pair<Builder&, StateIdx> add_state(StateIndices const& to = {}, bool accepting = false) {
                for(auto p: _transitions) {
                    p.second.emplace_back(to);
                }
                _unknown_transitions.emplace_back(to);
                _epsilon_transitions.emplace_back();
                _accepting_states.push_back(accepting);
                return { *this, state_count()-1 };
            }

            std::pair<Builder&, StateIdx> add_state(StateIdx const& to, bool accepting = false) {
                return add_state(StateIndices{to}, accepting);
            }

            std::pair<Builder&, StateIdx> add_state(bool accepting) {
                return add_state(StateIndices{}, accepting);
            }

            Builder& set_acceptance(StateIdx const& state, bool value) {
                check_state(state);
                _accepting_states[state] = value;
                return *this;
            }

            template<range R>
            Builder& set_acceptance(R&& states, bool value) {
                for(auto state: states) {
                    check_state(state);
                    _accepting_states[state] = value;
                }
                return *this;
            }

            Builder& set_acceptance(std::initializer_list<T> states, bool value) {
                return set_acceptance<std::initializer_list<T>&>(states, value);
            }

            Builder& add_transition(StateIdx const& state, T const& input, StateIdx const& to) {
                check_state(state);
                check_input(input);
                _transitions[input][state].insert(to);
                return *this;
            }

            template<range R>
            Builder& add_transitions(StateIdx const& state, T const& input, R&& to) {
                check_state(state);
                check_input(input);
                _transitions[input][state].insert(to.begin(), to.end());
                return *this;
            }

            template<range R>
            Builder& add_epsilon_transitions(StateIdx const& state, R&& to) {
                check_state(state);
                _epsilon_transitions[state].insert(to.begin(), to.end());
                return *this;
            }

            Builder& add_epsilon_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_epsilon_transitions<std::initializer_list<StateIdx>&>(state, to);
            }

            Builder& add_epsilon_transition(StateIdx const& state, StateIdx to) {
                check_state(state);
                _epsilon_transitions[state].insert(to);
                return *this;
            }

            template<range R>
            Builder& add_unknown_transitions(StateIdx const& state, R&& to) {
                check_state(state);
                _unknown_transitions[state].insert(to.begin(), to.end());
                return *this;
            }

            Builder& add_unknown_transitions(StateIdx const& state, std::initializer_list<StateIdx> to) {
                return add_unknown_transitions<std::initializer_list<StateIdx>&>(state, to);
            }

            Builder& add_unknown_transition(StateIdx const& state, StateIdx to) {
                check_state(state);
                _unknown_transitions[state].insert(to);
                return *this;
            }

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

            std::pair<Builder&, StateIdx> meld(Builder const& that) {
                for(auto input: that.inputs()) {
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
                        if(state[i] && accepting_states()[i]) {
                            return true;
                        }
                    }
                    return false;
                };
                
                std::vector<bool> trash(state_count(), false);
                std::vector<bool> start(trash);
                start[0] = true;

                typename DFA<T>::Builder builder(inputs);
                std::unordered_map<std::vector<bool>, typename DFA<T>::StateIdx> indices;
                indices.emplace(start, builder.add_state().second);
                indices.emplace(trash, builder.add_state().second);

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

                builder.complete(indices[trash]);

                return builder;
            }

            DFA<T>::Builder make_deterministic() const {
                return Builder(*this).make_deterministic();
            }

            operator DFA<T>::Builder() const {
                return make_deterministic();
            } 
        };

    };
}