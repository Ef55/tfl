#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <variant>
#include <iterator>
#include <concepts>

#include "Concepts.hpp"

namespace tfl {
    
    template<typename> class Regex;

    template<typename T, typename R>
    class RegexMatcher {
    protected:
        virtual ~RegexMatcher() = default;

    public:
        virtual R empty() const = 0;
        virtual R epsilon() const = 0;
        virtual R literal(std::function<bool(T)> const& predicate) const = 0;
        virtual R disjunction(Regex<T> const& left, Regex<T> const& right) const = 0;
        virtual R sequence(Regex<T> const& left, Regex<T> const& right) const = 0;
        virtual R kleene_star(Regex<T> const& regex) const = 0;
    };

    template<typename T>
    class Regex {
        
        class IsMatcher: public RegexMatcher<T, bool> {
            virtual bool empty() const { return false; }
            virtual bool epsilon() const { return false; }
            virtual bool literal(std::function<bool(T)> const& predicate) const { return false; }
            virtual bool disjunction(Regex<T> const& left, Regex<T> const& right) const { return false; }
            virtual bool sequence(Regex<T> const& left, Regex<T> const& right) const { return false; }
            virtual bool kleene_star(Regex<T> const& regex) const { return false; }
        };

        static class IsEmpty: public IsMatcher {
            bool empty() const { return true; }
        } constexpr is_empty{};

        static class IsEpsilon: public IsMatcher {
            bool epsilon() const { return true; }
        } constexpr is_epsilon{};

        struct Empty {
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.empty(); }
        };

        struct Epsilon {
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.epsilon(); }
        };

        struct Literal {
            std::function<bool(T)> const _pred;
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.literal(_pred); }
        };

        struct Disjunction {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.disjunction(_left, _right); }
        };

        struct Sequence {
            Regex const _left;
            Regex const _right;
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.sequence(_left, _right); }
        };

        struct KleeneStar {
            Regex const _underlying;
            template<typename R> inline R match(RegexMatcher<T, R> const& matcher) const { return matcher.kleene_star(_underlying); }
        };

        using Variant = std::variant<Empty, Epsilon, Literal, Disjunction, Sequence, KleeneStar>;
        std::shared_ptr<Variant> _regex;

        template<typename R> requires is_among_v<R, Empty, Epsilon, Literal, Disjunction, Sequence, KleeneStar>
        Regex(R&& regex): _regex(std::make_shared<Variant>(std::forward<R>(regex))) {}

    public:
        using TokenType = T;

        template<typename R>
        R match(RegexMatcher<T, R> const& matcher) const {
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

        static Regex literal(T const& lit) {
            return Regex{Literal([lit](T const& elem){ return elem == lit; })};
        }

        template<std::predicate<T> F>
        static Regex literal(F&& predicate) {
            return Regex{Literal(std::forward<F>(predicate))};
        }

        Regex operator| (Regex const& that) const {
            if(this->match(is_empty)) {
                return that;
            }
            else if(that.match(is_empty)) {
                return *this;
            }
            else {
                return Regex(Disjunction{*this, that});
            }
        }

        Regex operator& (Regex const& that) const {
            if(this->match(is_empty) || that.match(is_empty)) {
                return empty();
            }
            else if(this->match(is_epsilon)) {
                return that;
            }
            else if(that.match(is_epsilon)) {
                return *this;
            }
            else {
                return Regex(Sequence{*this, that});
            }
        }

        Regex operator* () const {
            return Regex(KleeneStar(*this));
        }

        Regex operator+ () const {
            return *this & Regex(KleeneStar(*this));
        }
    };

    template<class T>
    struct RegexesDerivation {
    private:
        static class: public RegexMatcher<T, bool> {
            auto nullable(Regex<T> const& regex) const{ return regex.match(*this); }

        public:
            bool empty() const {
                return false;
            }

            bool epsilon() const {
                return true;
            }
            
            bool literal(std::function<bool(T)> const& predicate) const {
                return false;
            }
            
            bool disjunction(Regex<T> const& left, Regex<T> const& right) const {
                return nullable(left) || nullable(right);
            }
            
            bool sequence(Regex<T> const& left, Regex<T> const& right) const {
                return nullable(left) && nullable(right);
            }
            
            bool kleene_star(Regex<T> const& regex) const {
                return true;
            }  
        } constexpr nullability_checker{};

        class RegexDeriver: public RegexMatcher<T, Regex<T>> {
            T const& _x;

            auto derive(Regex<T> const& regex) const { return regex.match(*this); }
        public:
            RegexDeriver(T const& x): _x(x) {}

            Regex<T> empty() const {
                return Regex<T>::empty();
            }

            Regex<T> epsilon() const {
                return Regex<T>::empty();
            }
            
            Regex<T> literal(std::function<bool(T)> const& predicate) const {
                return predicate(_x) ? Regex<T>::epsilon() : Regex<T>::empty();
            }
            
            Regex<T> disjunction(Regex<T> const& left, Regex<T> const& right) const {
                return derive(left) | derive(right);
            }
            
            Regex<T> sequence(Regex<T> const& left, Regex<T> const& right) const {
                auto d = derive(left) & right;
                return nullable(left) ? d | derive(right) : d;
            }
            
            Regex<T> kleene_star(Regex<T> const& regex) const {
                return derive(regex) & *regex;
            }  
        };

    public:
        static bool nullable(Regex<T> const& regex) {
            return regex.match(nullability_checker);
        }

        static Regex<T> derive(T const& x, Regex<T> const& regex) {
            return regex.match(RegexDeriver{x});
        }

        template<std::input_iterator It>
        static Regex<T> derive(It beg, It end, Regex<T> const& regex) {
            auto res = regex;
            for(; beg != end; ++beg) {
                res = derive(*beg, res);
            }

            return res;
        }

        template<std::input_iterator It>
        static bool accepts(Regex<T> const& regex, It beg, It end) {
            return nullable(derive(beg, end, regex));
        }

        static bool accepts(Regex<T> const& regex, std::initializer_list<T> ls) {
            return nullable(derive(ls.begin(), ls.end(), regex));
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

        static Regex<T> literal(T const& lit) {
            return Regex<T>::literal(lit);
        }

        template<std::predicate<T> F>
        static Regex<T> literal(F&& predicate) {
            return Regex<T>::literal(std::forward<F>(predicate));
        }

        static Regex<T> opt(Regex<T> const& r) {
            return epsilon() | r;
        }

        template<iterable<T> C>
        static Regex<T> word(C const& iterable) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : iterable) {
                result = result & Regex<T>::literal(lit);
            }

            return result;
        }

        static Regex<T> word(std::initializer_list<T> const& iterable) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : iterable) {
                result = result & Regex<T>::literal(lit);
            }

            return result;
        }

        static Regex<T> any_literal() {
            return Regex<T>::literal([](auto){ return true; });
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

        static Regex<T> range(T const& low, T const& high) {
            return Regex<T>::literal([low, high](auto chr){ return low <= chr && chr <= high; });
        }
    };
}
