#pragma once

#include <string>

#include "Stringify.hpp"
#include "Concepts.hpp"

namespace tfl {
    template<typename> class Regex;

    namespace matchers {

        template<typename T, typename R>
        class Base {
        protected:
            virtual ~Base() = default;

        public:
            virtual inline R rec(Regex<T> const& regex) const final { return regex.match(*this); };
            virtual inline R operator()(Regex<T> const& regex) const final { return rec(regex); }

            virtual R empty() const = 0;
            virtual R epsilon() const = 0;
            virtual R alphabet() const = 0;
            virtual R literal(T const& literal) const = 0;
            virtual R disjunction(Regex<T> const& left, Regex<T> const& right) const = 0;
            virtual R sequence(Regex<T> const& left, Regex<T> const& right) const = 0;
            virtual R kleene_star(Regex<T> const& regex) const = 0;
            virtual R complement(Regex<T> const& regex) const = 0;
            virtual R conjunction(Regex<T> const& left, Regex<T> const& right) const = 0;
        };

        template<typename T>
        class IsNone: public Base<T, bool> {
            virtual bool empty() const { return false; }
            virtual bool epsilon() const { return false; }
            virtual bool alphabet() const { return true; }
            virtual bool literal(T const& literal) const { return false; }
            virtual bool disjunction(Regex<T> const& left, Regex<T> const& right) const { return false; }
            virtual bool sequence(Regex<T> const& left, Regex<T> const& right) const { return false; }
            virtual bool kleene_star(Regex<T> const& regex) const { return false; }
            virtual bool complement(Regex<T> const& regex) const { return false; }
            virtual bool conjunction(Regex<T> const& left, Regex<T> const& right) const { return false; }
        };

        namespace {

            template<typename T>
            class IsEmpty final: public IsNone<T> {
                bool empty() const { return true; }
            };
            template<typename T> constexpr IsEmpty<T> is_empty{};

            template<typename T>
            class IsEpsilon final: public IsNone<T> {
                bool epsilon() const { return true; }
            };
            template<typename T> constexpr IsEpsilon<T> is_epsilon{};

            template<typename T>
            class IsAlphabet final: public IsNone<T> {
                bool alphabet() const { return true; }
            };
            template<typename T> constexpr IsAlphabet<T> is_alphabet{};

            template<typename T>
            class IsKleene final: public IsNone<T> {
                bool kleene_star(Regex<T> const&) const { return true; }
            };
            template<typename T> constexpr IsKleene<T> is_kleene_star{};

            template<typename T>
            class IsComplement final: public IsNone<T> {
                bool complement(Regex<T> const&) const { return true; }
            };
            template<typename T> constexpr IsComplement<T> is_complement{};

            template<typename T>
            class IsAny final: public IsNone<T> {
                bool complement(Regex<T> const& regex) const { return regex.match(is_alphabet<T>); }
            };
            template<typename T> constexpr IsAny<T> is_any{};



            enum class Precedence {
                ATOM = 1,
                SEQ = 2,
                CONJ = 3,
                DISJ = 4,
            };
            template<typename T, typename Stringify>
            class Printer final: public Base<T, std::pair<std::string, Precedence>> {
                using Result = std::pair<std::string, Precedence>;
                using Base<T, Result>::rec;
                static constexpr Stringify stringify{};


                inline static std::string paren_if_gtr(Result const& r, Precedence pri) {
                    return r.second > pri ? '(' + r.first + ')' : r.first;
                }

                inline static std::string paren_if_geq(Result const& r, Precedence pri) {
                    return r.second >= pri ? '(' + r.first + ')' : r.first;
                }

                inline static Result unop_rightassoc(std::string const& op, Result const& r, Precedence pri) {
                    return {op + paren_if_gtr(r, pri), pri};
                }

                inline static Result binop_leftassoc(std::string const& op, Result const& l, Result const& r, Precedence pri) {
                    return {paren_if_gtr(l, pri) + op + paren_if_geq(r, pri), pri};
                }

            public:
                virtual Result empty() const { return {"∅", Precedence::ATOM}; }
                virtual Result epsilon() const { return {"ε", Precedence::ATOM}; }
                virtual Result alphabet() const { return {"Σ", Precedence::ATOM}; }
                virtual Result literal(T const& lit) const { return {stringify(lit), Precedence::ATOM}; }
                virtual Result disjunction(Regex<T> const& left, Regex<T> const& right) const { 
                    return binop_leftassoc(" | ", rec(left), rec(right), Precedence::DISJ);
                }
                virtual Result sequence(Regex<T> const& left, Regex<T> const& right) const {
                    return binop_leftassoc("", rec(left), rec(right), Precedence::SEQ);
                }
                virtual Result kleene_star(Regex<T> const& regex) const {
                    return unop_rightassoc("*", rec(regex), Precedence::ATOM);
                }
                virtual Result complement(Regex<T> const& regex) const {
                    return unop_rightassoc("¬", rec(regex), Precedence::ATOM);
                }
                virtual Result conjunction(Regex<T> const& left, Regex<T> const& right) const { 
                    return binop_leftassoc(" & ", rec(left), rec(right), Precedence::CONJ);
                }
            };
            template<typename T, typename Stringify> constexpr Printer<T, Stringify> printer{};



            template<typename T, class Eq>
            class NullabilityChecker final: public Base<T, bool> {
                using Base<T, bool>::rec;
            public:
                bool empty() const { return false; }
                bool epsilon() const { return true; }
                bool alphabet() const { return false; }
                bool literal(T const&) const { return false; }
                bool disjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) || rec(right); }
                bool sequence(Regex<T> const& left, Regex<T> const& right) const { return rec(left) && rec(right); }
                bool kleene_star(Regex<T> const&) const { return true; }  
                bool complement(Regex<T> const& regex) const { return !rec(regex); }  
                bool conjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) && rec(right); }
            };
            template<typename T, class Eq> constexpr NullabilityChecker<T, Eq> nullability_checker{};


            template<typename T, class Eq>
            class Deriver final: public Base<T, Regex<T>> {
                using Base<T, Regex<T>>::rec;
                static Eq constexpr eq{};
                T const& _x;
            public:
                Deriver(T const& x): _x(x) {}

                Regex<T> empty() const { return Regex<T>::empty(); }
                Regex<T> epsilon() const { return Regex<T>::empty(); }
                Regex<T> alphabet() const { return Regex<T>::epsilon(); }
                Regex<T> literal(T const& literal) const { return eq(literal, _x) ? Regex<T>::epsilon() : Regex<T>::empty(); }
                Regex<T> disjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) | rec(right); }
                Regex<T> sequence(Regex<T> const& left, Regex<T> const& right) const {
                    auto d = rec(left) - right;
                    return left.match(nullability_checker<T, Eq>) ? d | rec(right) : d;
                }
                Regex<T> kleene_star(Regex<T> const& regex) const { return rec(regex) - *regex; }
                Regex<T> complement(Regex<T> const& regex) const { return ~rec(regex); }  
                Regex<T> conjunction(Regex<T> const& left, Regex<T> const& right) const { return rec(left) & rec(right); }
            };
        }

        
    };

    template<typename T>
    inline bool is_empty(Regex<T> const& regex) {
        return regex.match(matchers::is_empty<T>);
    }

    template<typename T>
    inline bool is_epsilon(Regex<T> const& regex) {
        return regex.match(matchers::is_epsilon<T>);
    }

    template<typename T>
    inline bool is_alphabet(Regex<T> const& regex) {
        return regex.match(matchers::is_alphabet<T>);
    }

    template<typename T>
    inline bool is_kleene_star(Regex<T> const& regex) {
        return regex.match(matchers::is_kleene_star<T>);
    }

    template<typename T>
    inline bool is_complement(Regex<T> const& regex) {
        return regex.match(matchers::is_complement<T>);
    }

    template<typename T>
    inline bool is_any(Regex<T> const& regex) {
        return regex.match(matchers::is_any<T>);
    }

    template<typename T, class Stringify = Stringify<T>>
    inline std::string to_string(Regex<T> const& regex) {
        return regex.match(matchers::printer<T, Stringify>).first;
    }

    template<typename T, class Eq = std::equal_to<T>>
    inline bool is_nullable(Regex<T> const& regex) {
        return regex.match(matchers::nullability_checker<T, Eq>);
    }

    template<typename T, class Eq = std::equal_to<T>>
    inline Regex<T> derive(T const& x, Regex<T> const& regex) {
        return regex.match(matchers::Deriver<T, Eq>{x});
    }

    template<typename T, std::input_iterator It, class Eq = std::equal_to<T>>
    Regex<T> derive(It beg, It end, Regex<T> const& regex) {
        auto res = regex;
        for(; beg != end; ++beg) {
            res = derive<T, Eq>(*beg, res);
        }

        return res;
    }
}