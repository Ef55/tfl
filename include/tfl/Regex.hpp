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
     * @brief <a href="https://en.wikipedia.org/wiki/Regular_expression">Regular expression</a>.
     *
     * %Regexes (<b>Reg</b>ular <b>Ex</b>pression<b>s</b>) are a handy tool to define sets of strings, using some base sets and 5 operators.
     * More formally, a regex \f$ r \f$ on some alphabet \f$ \Sigma \f$ defines a set of strings \f$ \mathcal{L}(r) \subset \Sigma^* \f$
     * called its language.
     *
     * The constructors (basic sets and operators) are (format: *Name(arguments...): math-notation function-name*):
     * - *Empty*: \f$ \emptyset \f$ \ref empty();
     * - *Epsilon*: \f$ \varepsilon \f$ \ref epsilon();
     * - *Alphabet*: \f$ \Sigma \f$ \ref alphabet();
     * - *Literal(T)*: \f$ a \f$ \ref literal();
     * - *Disjunction(Regex, Regex)*: \f$ \mathbin{|} \f$ \ref operator|();
     * - *Sequence(Regex, Regex)*: \f$ \cdot \f$ \ref operator-();
     * - *KleeneStar(Regex)*: \f$ * \f$ \ref operator*();
     * - *Complement(Regex)*: \f$ \neg \f$ \ref operator~();
     * - *Conjunction(Regex, Regex)*: \f$ \mathbin{\&} \f$ \ref operator&().
     *
     * This class is implemented in a <a href="https://en.wikipedia.org/wiki/Algebraic_data_type">ADT</a>-like fashion. 
     * Pattern matching can be performed using \ref matchers::Base in conjunction with \ref Regex::match().
     *
     * Note that this implementation only sees regexes as a "binary-tree-like structure",
     * which define a language, and as such does not provide methods to test a string for language-membership.
     * This can then be performed using regex derivation, DFAs or NFAs.
     *
     * @note **The constructors are smart constructors**: \n
     * Two different regexes \f$ r_1 \not= r_2 \f$ are considered equivalent, noted \f$ r_1 \equiv r_2 \f$, if \f$ \mathcal{L}(r_1) = \mathcal{L}(r_2) \f$. \n 
     * The constructors are allowed (and will) sometimes return a different (yet equivalent) regex from the one which should have been built. \n 
     * The used equivalences will be specified in the documentation of each constructor.
     *
     * @tparam T Type of literals, \f$ \equiv \Sigma \f$.
     *
     * @see \ref RegexOps.hpp for regex derivation.
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
        * @brief Type of literals (Equivalent to template T)
        */
        using TokenType = T;

        
        /**
         * @name Constructors
         * @{
         */

        /**
         * @brief \f$ \mathcal{L} = \left\{ \right\} \f$
         */
        static Regex empty() {
            static Regex const empty{Empty{}};
            return empty;
        }

        /**
         * @brief \f$ \mathcal{L} = \left\{ \varepsilon \right\} \f$
         */
        static Regex epsilon() {
            static Regex epsilon{Epsilon{}};
            return epsilon;
        }

        /**
         * @brief \f$ \mathcal{L} = \Sigma \f$
         */
        static Regex alphabet() {
            static Regex alpha{Alphabet{}};
            return alpha;
        }

        /**
         * @brief \f$ \mathcal{L} = \left\{ a \right\} \f$
         */
        static Regex literal(T const& a) {
            return Regex{Literal(a)};
        }

        /**
         * @brief \f$ \mathcal{L} = \mathcal{L}(\textup{this}) \cup \mathcal{L}(\textup{that}) \f$
         *
         * **Used equivalences:**
         * - \f$ \emptyset \mathbin{|} r \equiv r \f$;
         * - \f$ r \mathbin{|} \emptyset \equiv r \f$;
         * - \f$ \Sigma^{*} \mathbin{|} r \equiv \Sigma^{*} \f$;
         * - \f$ r \mathbin{|} \Sigma^{*} \equiv \Sigma^{*} \f$
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
         * @brief \f$ \mathcal{L} = \left\{ vw \mid v \in \mathcal{L}(\textup{this}) \textup{ and } v \in \mathcal{L}(\textup{that}) \right\} \f$
         *
         * **Used equivalences:**
         * - \f$ \emptyset \cdot r \equiv \emptyset \f$;
         * - \f$ r \cdot \emptyset \equiv \emptyset \f$;
         * - \f$ \varepsilon \cdot r \equiv r \f$;
         * - \f$ r \cdot \varepsilon \equiv r \f$
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
         * @brief \f$ \mathcal{L} = \mathcal{L}(\textup{this})^{*} \f$
         *
         * **Used equivalences:**
         * - \f$ (r^{*})^{*} \equiv r^{*} \f$;
         * - \f$ \emptyset^{*} \equiv \varepsilon \f$;
         * - \f$ \varepsilon^{*} \equiv \varepsilon \f$;
         * - \f$ \Sigma^{*} \equiv \neg\emptyset \f$
         *
         * @see <a href="https://en.wikipedia.org/wiki/Kleene_star">Kleene closure</a> on sets.
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
         * @brief @brief \f$ \mathcal{L} = \Sigma^{*} \mathbin{\backslash} \mathcal{L}(\textup{this}) \f$
         *
         * **Used equivalences:**
         * - \f$ \neg\neg r \equiv r \f$;
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
         * @brief @brief \f$ \mathcal{L} = \mathcal{L}(\textup{this}) \cap \mathcal{L}(\textup{that}) \f$
         *
         * **Used equivalences:**
         * - \f$ \emptyset \mathbin{\&} r \equiv \emptyset \f$;
         * - \f$ r \mathbin{\&} \emptyset \equiv \emptyset \f$;
         * - \f$ \Sigma^{*} \mathbin{\&} r \equiv r \f$;
         * - \f$ r \mathbin{\&} \Sigma^{*} \equiv r \f$
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
         * @brief Those constructors are defined in terms of the basic ones, so not essentials, but provided for convenience.
         * @{
         */

        /**
         * @brief \f$ \Sigma^{*} := \neg\emptyset \f$
         */
        static Regex any() {
            static Regex const any{~empty()};
            return any;
        }

        /**
         * @brief \f$ r^{+} := r \cdot r^{*} \f$
         */
        Regex operator+() const {
            return *this - Regex(KleeneStar(*this));
        }

        /**
         * @brief \f$ r_{1} \mathbin{/} r_{2} \equiv r_{1} \mathbin{\&} \neg r_{2} \f$
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
        template<input_range_of<T> C>
        static Regex<T> word(C const& range) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : range) {
                result = result - Regex<T>::literal(lit);
            }

            return result;
        }

        /**
         * @brief Makes a regex which only accept the specified sequence.
         */
        static Regex<T> word(std::initializer_list<T> const& range) {
            return word<std::initializer_list<T>>(range);
        }

        /**
         * @brief Makes a regex which accepts any of the literal passed as argument.
         */
        template<input_range_of<T> C>
        static Regex<T> any_of(C const& range) {
            Regex<T> result = Regex<T>::empty();
            for(T const& lit : range) {
                result = result | Regex<T>::literal(lit);
            }

            return result;
        }

        /**
         * @brief Makes a regex which accepts any sequence accepted by one of the regex passed as argument.
         */
        template<input_range_of<Regex<T>> C>
        static Regex<T> any_of(C const& range) {
            Regex<T> result = Regex<T>::empty();
            for(Regex<T> const& regex : range) {
                result = result | regex;
            }

            return result;
        }

        /**
         * @brief Calls one of the other overloads of `any_of` depending on the type `C`.
         */
        template<typename C>
        static Regex<T> any_of(std::initializer_list<C> const& range) {
            return any_of<std::initializer_list<C>>(range);
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
