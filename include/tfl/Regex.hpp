#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <iterator>

namespace tfl {
    
    template<typename T>
    class Regex final {

        class RegexBase;
        std::shared_ptr<RegexBase> _regex;

        Regex(RegexBase* ptr): _regex(ptr) {}
        Regex(std::shared_ptr<RegexBase> const& ptr): _regex(ptr) {}

        class RegexBase {
        public:
            virtual ~RegexBase() = default;

            virtual Regex<T> derive(T const& x) const = 0;
            virtual bool nullable() const = 0;

            virtual bool isEmpty() const = 0;
            virtual bool isEpsilon() const = 0;
        };

        class Empty: public RegexBase {
        public:
            virtual Regex<T> derive(T const&) const {
                return Regex::empty();
            }

            virtual bool nullable() const {
                return false;
            }

            virtual bool isEmpty() const {
                return true;
            }

            virtual bool isEpsilon() const {
                return false;
            }
        };

        class Epsilon: public RegexBase {
        public:
            virtual Regex<T> derive(T const&) const {
                return Regex::empty();
            }

            virtual bool nullable() const {
                return true;
            }

            virtual bool isEmpty() const {
                return false;
            }

            virtual bool isEpsilon() const {
                return true;
            }
        };

        class Literal: public RegexBase {
            std::function<bool(T)> const _pred;
        public:
            Literal(std::function<bool(T)> const& predicate): _pred(predicate) {}

            virtual Regex<T> derive(T const& x) const {
                return _pred(x) ?
                    Regex::epsilon():
                    Regex::empty();
                
            }

            virtual bool nullable() const {
                return false;
            }

            virtual bool isEmpty() const {
                return false;
            }

            virtual bool isEpsilon() const {
                return false;
            }
        };

        class Disjunction: public RegexBase {
            Regex const _left;
            Regex const _right;

        public:
            Disjunction(Regex const& left, Regex const& right): _left(left), _right(right) {}

            virtual Regex<T> derive(T const& x) const {
                return _left.derive(x) | _right.derive(x);
            }

            virtual bool nullable() const {
                return _left.nullable() || _right.nullable();
            }

            virtual bool isEmpty() const {
                return false;
            }

            virtual bool isEpsilon() const {
                return false;
            }
        };

        class Sequence: public RegexBase {
            Regex const _left;
            Regex const _right;

        public:
            Sequence(Regex const& left, Regex const& right): _left(left), _right(right) {}

            virtual Regex<T> derive(T const& x) const {
                return _left.nullable() ?
                    (_left.derive(x) & _right) | _right.derive(x):
                    _left.derive(x) & _right;
            }

            virtual bool nullable() const {
                return _left.nullable() && _right.nullable();
            }

            virtual bool isEmpty() const {
                return false;
            }

            virtual bool isEpsilon() const {
                return false;
            }
        };

        class KleeneStar: public RegexBase {
            Regex const _underlying;

        public:
            KleeneStar(Regex const& underlying): _underlying(underlying) {}

            virtual Regex<T> derive(T const& x) const {
                return _underlying.derive(x) & *_underlying;
            }

            virtual bool nullable() const {
                return true;
            }

            virtual bool isEmpty() const {
                return false;
            }

            virtual bool isEpsilon() const {
                return false;
            }
        };

        bool isEmpty() const {
            return _regex->isEmpty();
        }

        bool isEpsilon() const {
            return _regex->isEpsilon();
        }

    public:

        Regex derive(T const& x) const {
            return _regex->derive(x);
        };

        bool nullable() const {
            return _regex->nullable();
        };

        template<class It>
        bool accepts(It beg, It end) const {
            Regex r = *this;
            for(; beg != end; ++beg) {
                r = r.derive(*beg);
            }
            return r.nullable();
        }

        bool accepts(std::initializer_list<T> init) const {
            std::vector<T> v(init);
            return accepts(v.cbegin(), v.cend());
        }

        static Regex empty() {
            static const Regex empty(new Empty());
            return empty;
        }

        static Regex epsilon() {
            static const Regex epsilon(new Epsilon());
            return epsilon;
        }

        static Regex literal(T const& lit) {
            return Regex(new Literal([lit](T const& elem){ return elem == lit; }));
        }

        template<typename F>
        static Regex literal(F predicate) {
            return Regex(new Literal(predicate));
        }

        Regex operator| (Regex const& that) const {
            if(that.isEmpty() || (nullable() && that.isEpsilon())) {
                return *this;
            }
            else if(isEmpty() || (that.nullable() && isEpsilon())) {
                return that;
            }
            else {
                return Regex(new Disjunction(*this, that));
            }
        }

        Regex operator& (Regex const& that) const {
            if(isEmpty() || that.isEmpty()) {
                return Regex::empty();
            }
            else if(isEpsilon()) {
                return that;
            }
            else if(that.isEpsilon()) {
                return *this;
            }
            else {
                return Regex(new Sequence(*this, that));
            }
        }

        Regex operator* () const {
            return Regex(new KleeneStar(*this));
        }

    };

    template<typename T>
    struct Regexes {
        Regexes() = delete;

        template<class Container>
        static Regex<T> word(Container const& container) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : container) {
                result = result & Regex<T>::literal(lit);
            }

            return result;
        }

        static Regex<T> word(std::initializer_list<T> const& container) {
            Regex<T> result = Regex<T>::epsilon();
            for(T const& lit : container) {
                result = result & Regex<T>::literal(lit);
            }

            return result;
        }

        template<class Container>
        static Regex<T> any_literal(Container const& container) {
            Regex<T> result = Regex<T>::empty();
            for(T const& lit : container) {
                result = result | Regex<T>::literal(lit);
            }

            return result;
        }

        static Regex<T> any_literal(std::initializer_list<T> const& container) {
            Regex<T> result = Regex<T>::empty();
            for(T const& lit : container) {
                result = result | Regex<T>::literal(lit);
            }

            return result;
        }

        template<class Container>
        static Regex<T> any(Container const& container) {
            Regex<T> result = Regex<T>::empty();
            for(Regex<T> const& regex : container) {
                result = result | regex;
            }

            return result;
        }

        static Regex<T> any(std::initializer_list<Regex<T>> const& container) {
            Regex<T> result = Regex<T>::empty();
            for(Regex<T> const& regex : container) {
                result = result | regex;
            }

            return result;
        }

        static Regex<T> range(T const& low, T const& high) {
            return Regex<T>::literal([low, high](auto chr){ return low <= chr && chr <= high; });
        }
    };
}
