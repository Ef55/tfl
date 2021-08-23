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

namespace tfl {
    
    template<typename T>
    class Regex final {
        struct Empty {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.empty(); }
        };

        struct Epsilon {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.epsilon(); }
        };

        struct Alphabet {
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.alphabet(); }
        };

        struct Literal {
            T const _lit;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.literal(_lit); }
        };

        struct Disjunction {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.disjunction(_left, _right); }
        };

        struct Sequence {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.sequence(_left, _right); }
        };

        struct KleeneStar {
            Regex const _underlying;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.kleene_star(_underlying); }
        };

        struct Complement {
            Regex const _underlying;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.complement(_underlying); }
        };

        struct Conjunction {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(matchers::Base<T, R> const& matcher) const { return matcher.conjunction(_left, _right); }
        };

        using Variant = std::variant<Empty, Epsilon, Alphabet, Literal, Disjunction, Sequence, KleeneStar, Complement, Conjunction>;
        std::shared_ptr<Variant> _regex;

        template<typename R> requires is_among_v<R, Empty, Epsilon, Alphabet, Literal, Disjunction, Sequence, KleeneStar, Complement, Conjunction>
        Regex(R&& regex): _regex(std::make_shared<Variant>(std::forward<R>(regex))) {}

    public:
        using TokenType = T;

        template<typename R>
        R match(matchers::Base<T, R> const& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_regex);
        }

        static Regex empty() {
            static Regex const empty{Empty{}};
            return empty;
        }

        static Regex epsilon() {
            static Regex epsilon{Epsilon{}};
            return epsilon;
        }

        static Regex alphabet() {
            static Regex alpha{Alphabet{}};
            return alpha;
        }

        static Regex literal(T const& lit) {
            return Regex{Literal(lit)};
        }

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

        Regex operator*() const {
            if(is_kleene_star(*this)) {
                return *this;
            }
            else if(is_empty(*this) || is_epsilon(*this)) {
                return epsilon();
            }
            else {
                return Regex(KleeneStar(*this));
            }
        }

        Regex operator~() const {
            if(is_complement(*this)) {
                return std::get<Complement>(*_regex)._underlying;
            }
            else {
                return Regex(Complement(*this));
            }
        }

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

        static Regex any() {
            static Regex const any{~empty()};
            return any;
        }

        Regex operator+() const {
            return *this - Regex(KleeneStar(*this));
        }

        Regex operator/(Regex const& that) const {
            return *this & ~that;
        }

    };

    template<typename T>
    struct RegexesMetrics {
    public: 
        using Size = std::size_t;

    private:
        static class: public matchers::Base<T, Size> {
            using matchers::Base<T, Size>::rec;
        public:
            Size empty() const { return 1; }
            Size epsilon() const { return 1; }
            Size alphabet() const { return 1; }
            Size literal(T const&) const { return 1; }
            Size disjunction(Regex<T> const& left, Regex<T> const& right) const { return std::max(rec(left), rec(right)) + 1; }
            Size sequence(Regex<T> const& left, Regex<T> const& right) const { return std::max(rec(left), rec(right)) + 1; }
            Size kleene_star(Regex<T> const& regex) const { return rec(regex) + 1; }
            Size complement(Regex<T> const& regex) const { return rec(regex) + 1; }
            Size conjunction(Regex<T> const& left, Regex<T> const& right) const { return std::max(rec(left), rec(right)) + 1; }
        } constexpr probe{};

        static class: public matchers::Base<T, Size> {
            using matchers::Base<T, Size>::rec;
        public:
            Size empty() const { return 1; }
            Size epsilon() const { return 1; }
            Size alphabet() const { return 1; }
            Size literal(T const&) const { return 1; }
            Size disjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) + rec(right) + 1; }
            Size sequence(Regex<T> const& left, Regex<T> const& right) const { return rec(left) + rec(right) + 1; }
            Size kleene_star(Regex<T> const& regex) const { return rec(regex) + 1; }
            Size complement(Regex<T> const& regex) const { return rec(regex) + 1; }
            Size conjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) + rec(right) + 1; }
        } constexpr measurer{};

    protected:
        virtual ~RegexesMetrics() = default;

    public:
        static Size depth(Regex<T> const& regex) {
            return probe(regex);
        }

        static Size size(Regex<T> const& regex) {
            return measurer(regex);
        }
    };

    template<typename T>
    struct Regexes {
    protected:
        Regexes() = default;

    public:

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

        static Regex<T> opt(Regex<T> const& r) {
            return epsilon() | r;
        }

        template<iterable<T> C>
        static Regex<T> word(C const& iterable) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : iterable) {
                result = result - Regex<T>::literal(lit);
            }

            return result;
        }

        static Regex<T> word(std::initializer_list<T> const& iterable) {
            return word<std::initializer_list<T>>(iterable);
        }

        static Regex<T> any() {
            return Regex<T>::any();
        }

        template<iterable<T> C>
        static Regex<T> any_of(C const& iterable) {
            Regex<T> result = Regex<T>::empty();
            for(T const& lit : iterable) {
                result = result | Regex<T>::literal(lit);
            }

            return result;
        }

        template<iterable<Regex<T>> C>
        static Regex<T> any_of(C const& iterable) {
            Regex<T> result = Regex<T>::empty();
            for(Regex<T> const& regex : iterable) {
                result = result | regex;
            }

            return result;
        }

        template<typename C>
        static Regex<T> any_of(std::initializer_list<C> const& iterable) {
            return any_of<std::initializer_list<C>>(iterable);
        }

        template<class eq = std::equal_to<T>, class less = std::less<T>>
        static Regex<T> range(T low, T const& high) {
            if(less{}(low, high)) {
                auto c = literal(low); // Variable needed, since c++ doesn't guarantee operands evaluation order
                return c | range(++low, high); 
            }
            else if(eq{}(low, high)) {
                return literal(low);
            }
            else {
                return empty();
            }
        }
    };
}
