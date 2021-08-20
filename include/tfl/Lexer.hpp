#pragma once

#include "Regex.hpp"

#include <vector>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <stdexcept>
#include <concepts>

#include "Concepts.hpp"

namespace tfl {

    class LexingException: public std::logic_error {
    public:
        LexingException(std::string const& what_arg): logic_error(what_arg) {}
    };

    template<typename T, typename, container<T>> class Lexer;
    template<typename T, typename, container<T>> class Rule;

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

    class LexerImpl final {
        LexerImpl() = delete;

        template<typename T, typename, container<T>> friend class Lexer;

        template<typename T, typename R, container<T> Word = std::vector<T>>
        class LexerBase {
        public:
            virtual ~LexerBase() = default;
            virtual std::vector<R> apply (std::vector<T>&) const = 0;
        };

        template<typename T, typename R, container<T> Word = std::vector<T>>
        class SimpleDerivationLexer final : public LexerBase<T, Positioned<R>, Word> {
            std::vector<Rule<T, R, Word>> _rules;
            Regex<T> _nl;

            template<
                std::input_iterator It, 
                typename I = typename std::iterator_traits<It>::difference_type, 
                class Res = std::optional<I>
            >
            static Res maximal(Regex<T> r, It beg, It end) {
                Res max = Res();
                I idx = 0;
                for(; beg != end; ++beg) {
                    ++idx;
                    r = RegexesDerivation<T>::derive(*beg, r);
                    if(RegexesDerivation<T>::nullable(r)) {
                        max = Res(idx);
                    }
                }

                return max;
            }

        public:

            SimpleDerivationLexer(std::initializer_list<Rule<T, R, Word>> rules, Regex<T> newline = Regex<T>::empty()): _rules(rules), _nl(newline) {}

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

                    Rule<T, R, Word> p = _rules[std::distance(current.begin(), r)];
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

        template<typename T, typename R, typename U, container<T> Word = std::vector<T>>
        class Map final: public LexerBase<T, R, Word> {
            std::function<R(U)> _map;
            Lexer<T, U, Word> _underlying;

        public:
            template<std::invocable<U> F>
            Map(Lexer<T, U, Word> const& underlying, F&& map): _map(std::forward<F>(map)), _underlying(underlying) {}

            virtual std::vector<R> apply (std::vector<T>& in) const {
                std::vector<U> sub(_underlying(in));
                std::vector<R> res(sub.size());
                std::transform(sub.cbegin(), sub.cend(), res.begin(), _map);
                return res;
            }
        };

        template<typename T, typename R, container<T> Word = std::vector<T>>
        class Filter final: public LexerBase<T, R, Word> {
            std::function<bool(R)> _filter;
            Lexer<T, R, Word> _underlying;

        public:
            template<std::predicate<R> F>
            Filter(Lexer<T, R, Word> const& underlying, F&& filter): _filter([filter](auto i){ return !filter(i); }), _underlying(underlying) {}

            virtual std::vector<R> apply (std::vector<T>& in) const {
                auto sub(_underlying(in));
                auto end = std::remove_if(sub.begin(), sub.end(), _filter);
                sub.erase(end, sub.cend());
                return sub;
            }
        };
    };

    template<typename T, typename R, container<T> Word = std::vector<T>>
    class Rule final {
        template<typename, typename, container<T>> friend class LexerImpl::SimpleDerivationLexer;
        using Map = std::function<R(Word)>;

        Regex<T> _regex;
        Map _map;

        template<std::input_iterator It>
        R map(It beg, It end) const {
            Word input(beg, end);
            return _map(input);
        }

        Regex<T> regex() const {
            return _regex;
        }

    public:
        template<std::invocable<Word> F>
        Rule(Regex<T> regex, F map): _regex(regex), _map(map) {}
    };

    template<typename T, typename R, container<T> Word = std::vector<T>>
    class Lexer final {
        template<typename S, typename, container<S>> friend class Lexer;
        std::shared_ptr<LexerImpl::LexerBase<T, R, Word>> _lexer;

        Lexer(LexerImpl::LexerBase<T, R, Word>* ptr): _lexer(ptr) {}

    public:

        static Lexer<T, Positioned<R>, Word> make_derivation_lexer(std::initializer_list<Rule<T, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return Lexer<T, Positioned<R>, Word>(new LexerImpl::SimpleDerivationLexer<T, R, Word>(rules, newline));
        }

        static Lexer<T, Positioned<R>, Word> make(std::initializer_list<Rule<T, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_derivation_lexer(rules, newline);
        }

        std::vector<R> operator() (std::vector<T>& input) const {
            return _lexer->apply(input);
        }

        template<std::input_iterator It>
        std::vector<R> operator() (It beg, It end) const {
            std::vector<T> input(beg, end);
            return _lexer->apply(input);
        }

        template<std::invocable<R> F, typename U = std::invoke_result_t<F, R>>
        Lexer<T, U, Word> map(F&& map) const {
            return Lexer<T, U, Word>(new LexerImpl::Map<T, U, R, Word>(*this, std::forward<F>(map)));
        }

        template<std::predicate<R> F>
        Lexer<T, R, Word> filter(F&& filter) const {
            return Lexer<T, R, Word>(new LexerImpl::Filter<T, R, Word>(*this, std::forward<F>(filter)));
        }

    };

}