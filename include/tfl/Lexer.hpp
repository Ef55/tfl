#pragma once

#include "Regex.hpp"

#include <vector>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <stdexcept>

namespace tfl {

    class LexingException: public std::logic_error {
    public:
        LexingException(std::string const& what_arg): logic_error(what_arg) {}
    };

    template<typename T>
    class Positioned {
        T _val;
        size_t _line;
        size_t _column;

    public:
        template<typename... Args>
        Positioned(size_t line, size_t column, Args&&... args): 
        _val(std::forward<Args>(args)...), _line(line), _column(column) {}

        T& value() {
            return _val;
        }

        T const& value() const {
            return _val;
        }

        size_t line() const {
            return _line;
        }

        size_t column() const {
            return _column;
        }

        bool operator== (Positioned const& that) const {
            return (this->_line == that._line)
                && (this->_column == that._column)
                && (this->_val == that._val);
        }
    };

    template<typename T, typename R>
    class Rule {
        template<typename U, typename V> friend class BaseLexer;
        using Map = std::function<R(std::vector<T>)>;

        Regex<T> _regex;
        Map _map;

        template<class It>
        R map(It beg, It end) const {
            std::vector<T> input(beg, end);
            return _map(input);
        }

        Regex<T> regex() const {
            return _regex;
        }
    public:
        template<typename F>
        Rule(Regex<T> regex, F map): _regex(regex), _map(map) {}
    };

    template<typename T, typename R>
    class LexerImpl {
    public:
        virtual std::vector<R> apply (std::vector<T>&) const = 0;
    };

    template<typename T, typename R>
    class BaseLexer: public LexerImpl<T, Positioned<R>> {

        std::vector<Rule<T, R>> _rules;
        Regex<T> _nl;

        template<
            class It, 
            typename I = typename std::iterator_traits<It>::difference_type, 
            class Res = std::optional<I>
        >
        static Res maximal(Regex<T> r, It beg, It end) {
            Res max = Res();
            I idx = 0;
            for(; beg != end; ++beg) {
                ++idx;
                r = r.derive(*beg);
                if(r.nullable()) {
                    max = Res(idx);
                }
            }

            return max;
        }

    public:

        BaseLexer(std::initializer_list<Rule<T, R>> rules, Regex<T> newline = Regex<T>::empty()): _rules(rules), _nl(newline) {}

        virtual std::vector<Positioned<R>> apply (std::vector<T>& input) const override {
            std::vector<Positioned<R>> output;
            auto cur = input.begin();

            size_t col = 1;
            size_t line = 1;

            std::vector current(_rules.size(), std::optional<size_t>());

            while(cur != input.end()) {
                std::transform(
                    _rules.begin(), 
                    _rules.end(), 
                    current.begin(), 
                    [&cur, &input](auto p){ return maximal(p.regex(), cur, input.end()); }
                );

                auto r = std::max_element(
                    current.begin(),
                    current.end(),
                    [](auto l, auto r) {
                        if(!l.has_value()) {
                            return true;
                        }
                        else if(!r.has_value()) {
                            return false;
                        }
                        else {
                            return r.value() > l.value();
                        }
                    }
                );

                if(!r->has_value()) {
                    throw LexingException("No rule applicable");
                }

                Rule<T, R> p = _rules[std::distance(current.begin(), r)];
                auto l = r->value();
                auto next = cur; 
                std::advance(next, l);
                output.push_back( Positioned<R>(line, col, p.map(cur, next)) );

                col += l;
                if(maximal(_nl, cur, input.end()).has_value()) {
                    col = 1;
                    line += 1;
                }

                cur = next;

            }
            
            return output;
        }

    };

    template<typename T, typename R>
    class Lexer final {
        template<typename U, typename V> friend class Lexer;
        std::shared_ptr<LexerImpl<T, R>> _lexer;

        Lexer(LexerImpl<T, R>* ptr): _lexer(ptr) {}

    public:

        // template<typename U, typename V>
        // static Lexer<U, Positioned<V>> make(std::initializer_list<Rule<U, V>> rules, Regex<U> newline = Regex<U>::empty()) {
        //     LexerImpl<U, Positioned<V>>* ptr = new BaseLexer<U, V>(rules, newline);
        //     return Lexer<U, Positioned<V>>(ptr);
        // }

        static Lexer<T, Positioned<R>> make(std::initializer_list<Rule<T, R>> rules, Regex<T> newline = Regex<T>::empty()) {
            LexerImpl<T, Positioned<R>>* ptr = new BaseLexer<T, R>(rules, newline);
            return Lexer<T, Positioned<R>>(ptr);
        }

        std::vector<R> operator() (std::vector<T>& input) const {
            return _lexer->apply(input);
        }

        template<class It>
        std::vector<R> operator() (It beg, It end) const {
            std::vector<T> input(beg, end);
            return _lexer->apply(input);
        }
    };

}