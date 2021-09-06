#pragma once

#include "Regex.hpp"
#include "Automata.hpp"
#include "AutomataOps.hpp"
#include "Concepts.hpp"

#include <vector>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <stdexcept>
#include <concepts>
#include <ranges>

/**
 * @brief Contains the definition of the \ref Lexer
 * @file
 */

namespace tfl {

    /**
     * @brief Exception thrown by the lexer.
     */
    class LexingException: public std::runtime_error {
    public:
        /**
         * @param what_arg Exception's text.
         */
        LexingException(std::string const& what_arg): runtime_error(what_arg) {}
    };

    template<typename T, typename, container<T>> class Lexer;
    template<typename T, typename, class> class Rule;

    /**
     * @brief Wraps a type and associated it with a start position.
     * 
     * @tparam T Wrapped type.
     * @todo Add an end position, as well as a token position.
     */
    template<typename T>
    class Positioned {
        T _val;
        size_t _line;
        size_t _column;

    public:
        /**
         * @brief Creates a positioned value.
         * 
         * @tparam Args Types of arguments to build the underyling `T`.
         * @param line The line where the value starts.
         * @param column The Column where the value starts.
         * @param args Arguments used to build the value.
         */
        template<typename... Args>
        Positioned(size_t line, size_t column, Args&&... args): 
        _val(std::forward<Args>(args)...), _line(line), _column(column) {}

        /**
         * @brief Returns a reference to the wrapped the value.
         */
        T& value() {
            return _val;
        }

        /**
         * @brief Returns a reference to the wrapped the value.
         */
        T const& value() const {
            return _val;
        }

        /**
         * @brief The line where the value starts.
         */
        size_t line() const {
            return _line;
        }

        /**
         * @brief The column where the value starts.
         */
        size_t column() const {
            return _column;
        }

        /**
         * @return True if the position and the wrapped value are equal.
         */
        bool operator== (Positioned const& that) const {
            return (this->_line == that._line)
                && (this->_column == that._column)
                && (this->_val == that._val);
        }
    };

    namespace {

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
            SimpleDerivationLexer(Range&& rules, Regex<T> newline = Regex<T>::empty()): 
            _rules(std::ranges::cbegin(rules), std::ranges::cend(rules)), _nl(newline) {}
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

    /**
     * @brief A lexing rule.
     * 
     * A rule is pair of constituted of
     * - A value of type `M` which is used to determine whether a sequence 
     * is accepted by the rule (typically a Regex or a DFA);
     * - A map function of type \f$ \textit{Word} \longmapsto R \f$ which is 
     * applied to the matched sequence.
     *
     * @tparam M The matcher type.
     * @tparam R The result type.
     * @tparam Word The buffer type.
     */
    template<typename M, typename R, class Word>
    class Rule final {
        template<typename T, typename, typename, container<T>> friend class SimpleLexerBase;
        template<typename T, typename, container<T>> friend class SimpleDFALexer;
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
        /**
         * @tparam F The map type.
         * @param matcher The matcher.
         * @param map The map.
         */
        template<std::invocable<Word> F>
        Rule(M const& matcher, F&& map): _matcher(matcher), _map(std::forward<F>(map)) {}
    };

    /**
     * @brief Tokenizes a sequence of characters.
     * 
     * Using provided regexes, the character sequence is
     * split into chunks which are then transformed into tokens.
     *
     * The process is as follows:
     * 1. Starting from the first character, every regex is applied to recognize
     * as many characters as possible;
     * 2. The map associated to the regex which matched the longest prefix
     * is applied to said prefix to generate a token;
     * 3. This process then starts over at the first character which was not 
     * part of the prefix, until there is no character left, or no regex matches anything.
     *
     * When two rules matched as many characters, the first rule is applied.
     *
     * @tparam T The character type.
     * @tparam R The token type.
     * @tparam Word The buffer type.
     */
    template<typename T, typename R, container<T> Word = std::vector<T>>
    class Lexer final {
        template<typename S, typename, container<S>> friend class Lexer;
        std::shared_ptr<LexerBase<T, R, Word>> _lexer;

        Lexer(LexerBase<T, R, Word>* ptr): _lexer(ptr) {}

    public:

        /**
         * @brief Generates a lexer where \ref derive(Regex) is used for language-membership testing.
         *
         * This lexer is quite inefficient, but really fast to build.
         * 
         * @param rules Rules specifying the lexer.
         * @param newline Regex defining a newline.
         */
        template<range_of<Rule<Regex<T>, R, Word>> Range>
        static Lexer<T, Positioned<R>, Word> make_derivation_lexer(Range&& rules, Regex<T> newline = Regex<T>::empty()) {
            return Lexer<T, Positioned<R>, Word>(new SimpleDerivationLexer<T, R, Word>(std::forward<Range>(rules), newline));
        }

        /**
         * @brief Generates a lexer where \ref DFA are used for language-membership testing.
         *
         * This lexer is way faster, but requires some time to build the DFAs.
         * 
         * @param rules Rules specifying the lexer.
         * @param newline Regex defining a newline.
         */
        template<range_of<Rule<Regex<T>, R, Word>> Range>
        static Lexer<T, Positioned<R>, Word> make_dfa_lexer(Range rules, Regex<T> newline = Regex<T>::empty()) {
            return Lexer<T, Positioned<R>, Word>(new SimpleDFALexer<T, R, Word>(std::forward<Range>(rules), newline));
        }

        /**
         * @brief Generates a lexer where \ref derive(Regex) is used for language-membership testing.
         */
        static Lexer<T, Positioned<R>, Word> make_derivation_lexer(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_derivation_lexer(views::all(rules), newline);
        }

        /**
         * @brief Generates a lexer where \ref DFA are used for language-membership testing.
         */
        static Lexer<T, Positioned<R>, Word> make_dfa_lexer(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_dfa_lexer(views::all(rules), newline);
        }

        /**
         * @brief Generates a lexer where \ref DFA are used for language-membership testing.
         * @see \ref make_dfa_lexer()
         * @deprecated You should use a function specifying which lexer to build
         */
        [[deprecated]]
        static Lexer<T, Positioned<R>, Word> make(std::initializer_list<Rule<Regex<T>, R, Word>> rules, Regex<T> newline = Regex<T>::empty()) {
            return make_dfa_lexer(rules, newline);
        }

        /**
         * @brief Applies the lexer to an input sequence.
         */
        std::vector<R> operator()(std::vector<T>& input) const {
            return _lexer->apply(input);
        }

        /**
         * @brief Applies the lexer to an input sequence.
         */
        template<std::ranges::range Range>
        std::vector<R> operator()(Range&& range) const {
            std::vector input(std::ranges::cbegin(range), std::ranges::cend(range));
            return _lexer->apply(input);
        }

        /**
         * @brief Applies a function to every generated token.
         */
        template<std::invocable<R> F, typename U = std::invoke_result_t<F, R>>
        Lexer<T, U, Word> map(F&& map) const {
            return Lexer<T, U, Word>(new Map<T, U, R, Word>(*this, std::forward<F>(map)));
        }

        /**
         * @brief Discards tokens for which the predicate is false.
         */
        template<std::predicate<R> F>
        Lexer<T, R, Word> filter(F&& filter) const {
            return Lexer<T, R, Word>(new Filter<T, R, Word>(*this, std::forward<F>(filter)));
        }

    };

}