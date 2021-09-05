#pragma once

#include "Regex.hpp"
#include "Automata.hpp"
#include "AutomataOps.hpp"

#include <vector>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <stdexcept>
#include <concepts>
#include <ranges>

#include "Concepts.hpp"

namespace tfl {

    class LexingException: public std::logic_error {
    public:
        LexingException(std::string const& what_arg): logic_error(what_arg) {}
    };

    template<typename T, typename, container<T>> class Lexer;
    template<typename T, typename, class> class Rule;

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

        template<typename T, class M, typename R, container<T> Word = std::vector<T>>
        class SimpleLexerBase : public LexerBase<T, Positioned<R>, Word> {
        protected:
            virtual std::vector<Rule<M, R, Word>> const& rules() const = 0;
            virtual M const& newline() const = 0;
            virtual std::optional<typename std::vector<T>::difference_type> maximal(M matcher, std::vector<T>::iterator beg, std::vector<T>::iterator end) const = 0;

        public:

            virtual std::vector<Positioned<R>> apply(std::vector<T>& input) const final override {
                std::vector<Positioned<R>> output;
                auto cur = input.begin();

                size_t col = 1;
                size_t line = 1;

                std::vector<Rule<M, R, Word>> const& rulz = rules();
                M const& nl = newline();
                std::vector current(rulz.size(), std::optional<size_t>());

                while(cur != input.end()) {
                    std::ranges::transform(
                        rulz,
                        current.begin(), 
                        [this, &cur, &input](auto p){ return maximal(p.matcher(), cur, input.end()); }
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

                    auto p = rulz[std::distance(current.begin(), r)];
                    auto l = r->value();
                    auto next = cur; 
                    std::advance(next, l);
                    output.push_back( Positioned<R>(line, col, p.map(cur, next)) );

                    col += l;
                    if(maximal(nl, cur, input.end()).has_value()) {
                        col = 1;
                        line += 1;
                    }

                    cur = next;

                }
                
                return output;
            }
        };
        
        template<typename T, typename R, container<T> Word = std::vector<T>>
        class SimpleDerivationLexer final : public SimpleLexerBase<T, Regex<T>, R, Word> {
            std::vector<Rule<Regex<T>, R, Word>> _rules;
            Regex<T> _nl;

        protected:
            std::vector<Rule<Regex<T>, R, Word>> const& rules() const override {
                return _rules;
            }

            Regex<T> const& newline() const override {
                return _nl;
            }

            std::optional<typename std::vector<T>::difference_type> maximal(Regex<T> regex, std::vector<T>::iterator beg, std::vector<T>::iterator end) const override {
                std::optional<typename std::vector<T>::difference_type> max = std::nullopt;
                typename std::vector<T>::difference_type idx = 0;
                for(; beg != end; ++beg) {
                    ++idx;

                    regex = derive(*beg, regex);
                    if(is_nullable(regex)) {
                        max = idx;
                    }
                }

                return max;
            }

        public:
            template<std::ranges::range Range>
            SimpleDerivationLexer(Range&& rules, Regex<T> newline = Regex<T>::empty()): _rules(rules.begin(), rules.end()), _nl(newline) {}
        };

        template<typename T, typename R, container<T> Word = std::vector<T>>
        class SimpleDFALexer final : public SimpleLexerBase<T, DFA<T>, R, Word> {
            std::vector<Rule<DFA<T>, R, Word>> _rules;
            DFA<T> _nl;

        protected:
            std::vector<Rule<DFA<T>, R, Word>> const& rules() const override {
                return _rules;
            }

            DFA<T> const& newline() const override {
                return _nl;
            }

            std::optional<typename std::vector<T>::difference_type> maximal(DFA<T> dfa, std::vector<T>::iterator beg, std::vector<T>::iterator end) const override {
                auto res = dfa.munch(std::ranges::subrange(beg, end));

                return res.has_value() && res.value() > 0
                    ? std::optional<typename std::vector<T>::difference_type>{res.value()} 
                    : std::optional<typename std::vector<T>::difference_type>{};
            }

        public:
            template<std::ranges::range Range>
            SimpleDFALexer(Range&& rules, Regex<T> newline = Regex<T>::empty()): 
            _rules(), 
            _nl{make_dfa(newline)} 
            {
                for(auto& rule: rules) {
                    _rules.emplace_back(
                        make_dfa(rule.matcher()),
                        rule._map
                    );
                }
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

    template<typename M, typename R, class Word>
    class Rule final {
        template<typename T, typename, typename, container<T>> friend class LexerImpl::SimpleLexerBase;
        template<typename T, typename, container<T>> friend class LexerImpl::SimpleDFALexer;
        using Map = std::function<R(Word)>;

        M _matcher;
        Map _map;

        template<std::input_iterator It>
        R map(It beg, It end) const {
            Word input(beg, end);
            return _map(input);
        }

        M matcher() const {
            return _matcher;
        }

    public:
        template<std::invocable<Word> F>
        Rule(M const& matcher, F map): _matcher(matcher), _map(map) {}
    };

    template<typename T, typename R, container<T> Word = std::vector<T>>
    class Lexer final {
        template<typename S, typename, container<S>> friend class Lexer;
        std::shared_ptr<LexerImpl::LexerBase<T, R, Word>> _lexer;

        Lexer(LexerImpl::LexerBase<T, R, Word>* ptr): _lexer(ptr) {}

    public:

        template<std::ranges::range Range> requires std::same_as<std::ranges::range_value_t<Range>, Rule<Regex<T>, R, Word>>
        static Lexer<T, Positioned<R>, Word> make_derivation_lexer(Range&& rules, Regex<T> newline = Regex<T>::empty()) {
            return Lexer<T, Positioned<R>, Word>(new LexerImpl::SimpleDerivationLexer<T, R, Word>(std::forward<Range>(rules), newline));
        }

        template<std::ranges::range Range> requires std::same_as<std::ranges::range_value_t<Range>, Rule<Regex<T>, R, Word>>
        static Lexer<T, Positioned<R>, Word> make_dfa_lexer(Range rules, Regex<T> newline = Regex<T>::empty()) {
            return Lexer<T, Positioned<R>, Word>(new LexerImpl::SimpleDFALexer<T, R, Word>(std::forward<Range>(rules), newline));
        }

        static Lexer<T, Positioned<R>, Word> make_derivation_lexer(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_derivation_lexer(views::all(rules), newline);
        }

        static Lexer<T, Positioned<R>, Word> make_dfa_lexer(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_dfa_lexer(views::all(rules), newline);
        }

        [[deprecated]]
        static Lexer<T, Positioned<R>, Word> make(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_dfa_lexer(rules, newline);
        }

        std::vector<R> operator()(std::vector<T>& input) const {
            return _lexer->apply(input);
        }

        template<std::ranges::range Range>
        std::vector<R> operator()(Range&& range) const {
            std::vector input(range.begin(), range.end());
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