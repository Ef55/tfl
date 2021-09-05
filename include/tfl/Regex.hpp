#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <variant>
#include <iterator>
#include <concepts>

#include "RegexOps.hpp"
#include "Stringify.hpp"
#include "Concepts.hpp"

/**
 * @brief Contains the definition of regexes.
 * @file 
 */

namespace tfl {
    
    /**
     * @brief %Regex.
     *
     * Implemented in a <a href="https://en.wikipedia.org/wiki/Algebraic_data_type">ADT</a>-like fashion. 
     * The "constructors" are:
     * - Empty;
     * - Epsilon;
     * - Alphabet;
     * - Literal(T);
     * - Disjunction(Regex, Regex);
     * - Sequence(Regex, Regex);
     * - KleeneStar(Regex);
     * - Complement(Regex);
     * - Conjunction(Regex, Regex).
     *
     * Pattern matching can be performed using \ref matchers::Base in conjunction with \ref Regex::match().
     *
     * Note that this implementation only sees regexes as a "binary-tree-like structure",
     * which define a set of accepted/rejected sequences, but not as a direct tool to match sequences of literals.
     * This can then be performed using regex derivation, DFAs or NFAs.
     *
     * More formally, a regex \f$ R \f$ defines a set of strings \f$ \mathcal{L}(R) \subset \Sigma^* \f$,
     * but no direct way to test whether \f$ s \in \mathcal{L}(R) \f$ where \f$ s \in \Sigma^* \f$.
     * 
     * @tparam T Type of literals.
     *
     * @see \ref derive() for regex derivation.
     * @see \ref DFA for DFAs.
     * @see \ref NFA for NFAs.
     *
     * \nosubgrouping
     */
    template<typename T>
    class Regex final {
        struct Empty {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.empty(); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.empty(); }
        };

        struct Epsilon {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.epsilon(); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.epsilon(); }
        };

        struct Alphabet {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.alphabet(); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.alphabet(); }
        };

        struct Literal {
            T const _lit;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.literal(_lit); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.literal(_lit); }
        };

        struct Disjunction {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.disjunction(_left, _right); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.disjunction(_left, _right); }
        };

        struct Sequence {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.sequence(_left, _right); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.sequence(_left, _right); }
        };

        struct KleeneStar {
            Regex const _underlying;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.kleene_star(_underlying); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.kleene_star(_underlying); }
        };

        struct Complement {
            Regex const _underlying;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.complement(_underlying); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.complement(_underlying); }
        };

        struct Conjunction {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.conjunction(_left, _right); }
            template<typename R> inline R match(matchers::MutableBase<T, R>& matcher) const { return matcher.conjunction(_left, _right); }
        };

        using Variant = std::variant<Empty, Epsilon, Alphabet, Literal, Disjunction, Sequence, KleeneStar, Complement, Conjunction>;
        std::shared_ptr<Variant> _regex;

        template<typename R> requires is_among_v<R, Empty, Epsilon, Alphabet, Literal, Disjunction, Sequence, KleeneStar, Complement, Conjunction>
        Regex(R&& regex): _regex(std::make_shared<Variant>(std::forward<R>(regex))) {}

    public:
        /**
        * @brief Type of literals (Equivalent to template T).
        */
        using TokenType = T;

        
        /**
         * @name Constructors
         * @{
         */

        /**
         * @brief Constructs an empty regex.
         */
        static Regex empty() {
            static Regex const empty{Empty{}};
            return empty;
        }

        /**
         * @brief Constructs an epsilon regex.
         */
        static Regex epsilon() {
            static Regex epsilon{Epsilon{}};
            return epsilon;
        }

        /**
         * @brief Constructs an alphabet regex.
         */
        static Regex alphabet() {
            static Regex alpha{Alphabet{}};
            return alpha;
        }

        /**
         * @brief Constructs a literal regex.
         */
        static Regex literal(T const& lit) {
            return Regex{Literal(lit)};
        }

        /**
         * @brief Constructs the disjunction of `this` and `that` regex.
         */
        Regex operator|(Regex const& that) const {
            if(is_empty(*this) || is_any(that)) {
                return that;
            }
            else if(is_empty(that) || is_any(*this)) {
                return *this;
            }
            else {
                return Regex(Disjunction{*this, that});
            }
        }

        /**
         * @brief Constructs the sequence of `this` and `that` regex.
         */
        Regex operator-(Regex const& that) const {
            if(is_empty(*this) || is_empty(that)) {
                return empty();
            }
            else if(is_epsilon(*this)) {
                return that;
            }
            else if(is_epsilon(that)) {
                return *this;
            }
            else {
                return Regex(Sequence{*this, that});
            }
        }

        /**
         * @brief Constructs the closure/repetition/(kleene)star of `this` regex.
         */
        Regex operator*() const {
            if(is_kleene_star(*this)) {
                return *this;
            }
            else if(is_empty(*this) || is_epsilon(*this)) {
                return epsilon();
            }
            else if(is_alphabet(*this)) {
                return any();
            }
            else {
                return Regex(KleeneStar(*this));
            }
        }

        /**
         * @brief Constructs the complement of `this` regex.
         */
        Regex operator~() const {
            if(is_complement(*this)) {
                return std::get<Complement>(*_regex)._underlying;
            }
            else {
                return Regex(Complement(*this));
            }
        }

        /**
         * @brief Constructs the conjunction of `this` and `that` regex.
         */
        Regex operator&(Regex const& that) const {
            if(is_empty(*this) || is_empty(that)) {
                return empty();
            }
            else if(is_any(*this)) {
                return that;
            }
            else if(is_any(that)) {
                return *this;
            }
            else {
                return Regex(Conjunction{*this, that});
            }
        }
        ///@}


        /**
         * @name Matching functions
         * @brief Apply a matcher to `this` regex.
         *
         * A matcher must inherit from either 
         * \ref matchers::Base or \ref matchers::MutableBase (see the latter for the difference between them).
         *
         * A matcher implements a function for each regex constructor. When \ref match() is called, 
         * the function of `matcher` corresponding to the constructor used to build `this` will be called
         * with the arguments used to build `this`.
         *
         * @{ 
         */

        template<typename R>
        R match(matchers::Base<T, R> const& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_regex);
        }

        template<typename R>
        R match(matchers::MutableBase<T, R>& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_regex);
        }

        template<typename R>
        R match(matchers::MutableBase<T, R>&& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_regex);
        }
        ///@}


        /**
         * @name Additional constructors
         * @{
         */

        /**
         * @brief Constructs a regex matching any sequence.
         *
         * \f[ \Sigma^{*} = \neg \emptyset \f] 
         */
        static Regex any() {
            static Regex const any{~empty()};
            return any;
        }

        /**
         * @brief Constructs the kleene plus of `this` regex.
         *
         * \f[ r^{+} \equiv rr^{*} \f]
         */
        Regex operator+() const {
            return *this - Regex(KleeneStar(*this));
        }

        /**
         * @brief Constructs the substraction of `that` regex from `this` regex.
         *
         * \f[ r_{1} \mathbin{/} r_{2} \equiv r_{1} \mathbin{\&} \neg r_{2} \f]
         */
        Regex operator/(Regex const& that) const {
            return *this & ~that;
        }
        ///@}

    };


    /**
     * @brief "Ghost class" containing functions to build regexes. 
     * 
     * This class is meant to be privately inherited to bring all
     * functions into scope.
     *
     * @tparam T Type of literals.
     */
    template<typename T>
    struct Regexes {
    protected:
        Regexes() = default;

    public:

        /**
         * @name Base constructors
         * @see The constructors in \ref Regex with the same name 
         * @{
         */

        static Regex<T> empty() {
            return Regex<T>::empty();
        }

        static Regex<T> epsilon() {
            return Regex<T>::epsilon();
        }

        static Regex<T> alphabet() {
            return Regex<T>::alphabet();
        }

        static Regex<T> literal(T const& lit) {
            return Regex<T>::literal(lit);
        }

        static Regex<T> any() {
            return Regex<T>::any();
        }
        ///@}

        /**
         * @name Additional combinators
         * @{
         */

        /**
         * @brief Makes the regex accept ε.
         */
        static Regex<T> opt(Regex<T> const& r) {
            return epsilon() | r;
        }

        /**
         * @brief Makes a regex which only accept the specified sequence.
         */
        template<range_of<T> C>
        static Regex<T> word(C const& range_of) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : range_of) {
                result = result - Regex<T>::literal(lit);
            }

            return result;
        }

        /**
         * @brief Makes a regex which only accept the specified sequence.
         */
        static Regex<T> word(std::initializer_list<T> const& range_of) {
            return word<std::initializer_list<T>>(range_of);
        }

        /**
         * @brief Makes a regex which accepts any of the literal passed as argument.
         */
        template<range_of<T> C>
        static Regex<T> any_of(C const& range_of) {
            Regex<T> result = Regex<T>::empty();
            for(T const& lit : range_of) {
                result = result | Regex<T>::literal(lit);
            }

            return result;
        }

        /**
         * @brief Makes a regex which accepts any sequence accepted by one of the regex passed as argument.
         */
        template<range_of<Regex<T>> C>
        static Regex<T> any_of(C const& range_of) {
            Regex<T> result = Regex<T>::empty();
            for(Regex<T> const& regex : range_of) {
                result = result | regex;
            }

            return result;
        }

        /**
         * @brief Calls one of the other overloads of `any_of` depending on the type `C`.
         */
        template<typename C>
        static Regex<T> any_of(std::initializer_list<C> const& range_of) {
            return any_of<std::initializer_list<C>>(range_of);
        }

        /**
         * @brief Makes a regex accepting any literal in range.
         * 
         * This function requires that prefix `operator++` is defined on type `T`.
         *
         * @tparam eq Function-object defining literal equality.
         * @tparam less Function-object defining literal "less-than".
         * @param low Range lower-bound (included).
         * @param high Range upper-bound (included).
         */
        template<class Eq = std::equal_to<T>, class Less = std::less<T>>
        static Regex<T> range(T low, T const& high) {
            if(Less{}(low, high)) {
                auto c = literal(low); // Variable needed, since c++ doesn't guarantee operands evaluation order
                return c | range(++low, high); 
            }
            else if(Eq{}(low, high)) {
                return literal(low);
            }
            else {
                return empty();
            }
        }
        ///@}
    };
}
