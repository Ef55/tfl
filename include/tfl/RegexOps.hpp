#pragma once

#include <string>
#include <set>
#include <ranges>

#include "Stringify.hpp"
#include "Concepts.hpp"

/**
 * @brief Contains additional operations on regexes.
 * @file 
 */

namespace tfl {
    template<typename> class Regex;

    /**
     * @brief Contains some basic matchers for regexes.
     */
    namespace matchers {

        /**
         * @brief %Base of all (non-mutable) regex matchers.
         * 
         * All methods will return the value resulting from the match operation.
         *
         * @tparam T The input type of the regexes.
         * @tparam R The returned type of the matcher.
         *
         * @see \ref MutableBase for matchers with mutable internal state.
         * @see \ref Regex::match
         */
        template<typename T, typename R>
        class Base {
        protected:
            virtual ~Base() = default;

        public:
            /**
             * @brief Matches a regex with this matcher.
             * 
             * @param regex The regex on which match.
             */
            virtual inline R rec(Regex<T> const& regex) const final { return regex.match(*this); };

            /**
             * @brief Matches a regex with this matcher.
             *
             * Equivalent to \ref rec.
             * 
             * @param regex The regex on which match.
             */
            virtual inline R operator()(Regex<T> const& regex) const final { return rec(regex); }

            /**
             * @brief Matches the empty regex.
             */
            virtual R empty() const = 0;

            /**
             * @brief Matches the epsilon regex.
             */
            virtual R epsilon() const = 0;
            
            /**
             * @brief Matches the alphabet regex.
             */
            virtual R alphabet() const = 0;
            
            /**
             * @brief Matches a literal regex.
             *
             * @param literal The literal the regex accepts.
             */
            virtual R literal(T const& literal) const = 0;
            
            /**
             * @brief Matches the disjunction of two regexes.
             *
             * @param left The left part of the disjunction.
             * @param right The right part of the disjunction.
             */
            virtual R disjunction(Regex<T> const& left, Regex<T> const& right) const = 0;
            
            /**
             * @brief Matches the sequence of two regexes.
             *
             * @param left The left part of the sequence.
             * @param right The right part of the sequence.
             */
            virtual R sequence(Regex<T> const& left, Regex<T> const& right) const = 0;
            
            /**
             * @brief Matches the closure/repetition/(kleene)star of a regex.
             *
             * @param regex The closed regex.
             */
            virtual R kleene_star(Regex<T> const& regex) const = 0;
            
            /**
             * @brief Matches the complement of a regex.
             *
             * @param regex The complemented regex.
             */
            virtual R complement(Regex<T> const& regex) const = 0;
            
            /**
             * @brief Matches the conjunction of two regexes.
             *
             * @param left The left part of the conjunction.
             * @param right The right part of the conjunction.
             */
            virtual R conjunction(Regex<T> const& left, Regex<T> const& right) const = 0;
        };

        /**
         * @brief %Base of all (mutable) regex matchers.
         * 
         * This class behaves exactly like \ref Base, 
         * except that matchers deriving MutableBase can
         * hold some mutable internal state, which might be required to implement some matchers.
         *
         * @tparam T The input type of the regexes.
         * @tparam R The returned type of the matcher.
         *
         * @see \ref Base for a complete documentation of all members.
         */
        template<typename T, typename R>
        class MutableBase {
        protected:
            virtual ~MutableBase() = default;

        public:
            virtual inline R rec(Regex<T> const& regex) final { return regex.match(*this); };
            virtual inline R operator()(Regex<T> const& regex) final { return rec(regex); }

            virtual R empty() = 0;
            virtual R epsilon() = 0;
            virtual R alphabet() = 0;
            virtual R literal(T const& literal) = 0;
            virtual R disjunction(Regex<T> const& left, Regex<T> const& right) = 0;
            virtual R sequence(Regex<T> const& left, Regex<T> const& right) = 0;
            virtual R kleene_star(Regex<T> const& regex) = 0;
            virtual R complement(Regex<T> const& regex) = 0;
            virtual R conjunction(Regex<T> const& left, Regex<T> const& right) = 0;
        };

        /**
         * @brief Base for boolean matchers.
         *
         * Returns `false` for any regex.
         */
        template<typename T>
        class IsNone: public Base<T, bool> {
            virtual bool empty() const { return false; }
            virtual bool epsilon() const { return false; }
            virtual bool alphabet() const { return false; }
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
                bool kleene_star(Regex<T> const& regex) const { return regex.match(is_alphabet<T>); }
                bool complement(Regex<T> const& regex) const { return regex.match(is_empty<T>); }
            };
            template<typename T> constexpr IsAny<T> is_any{};


            /// \private
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
                Result empty() const { return {"∅", Precedence::ATOM}; }
                Result epsilon() const { return {"ε", Precedence::ATOM}; }
                Result alphabet() const { return {"Σ", Precedence::ATOM}; }
                Result literal(T const& lit) const { return {Stringify::convert(lit), Precedence::ATOM}; }
                Result disjunction(Regex<T> const& left, Regex<T> const& right) const { 
                    return binop_leftassoc(" | ", rec(left), rec(right), Precedence::DISJ);
                }
                Result sequence(Regex<T> const& left, Regex<T> const& right) const {
                    return binop_leftassoc("", rec(left), rec(right), Precedence::SEQ);
                }
                Result kleene_star(Regex<T> const& regex) const {
                    return unop_rightassoc("*", rec(regex), Precedence::ATOM);
                }
                Result complement(Regex<T> const& regex) const {
                    return unop_rightassoc("¬", rec(regex), Precedence::ATOM);
                }
                Result conjunction(Regex<T> const& left, Regex<T> const& right) const { 
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



            template<typename T, class R>
            class AlphabetFinder final: public Base<T, R> {
                using Base<T, R>::rec;
            public:

                R empty() const { return R{}; }
                R epsilon() const { return R{}; }
                R alphabet() const { return R{}; }
                R literal(T const& literal) const  { return R{literal}; }
                R disjunction(Regex<T> const& left, Regex<T> const& right) const { 
                    R l(rec(left));
                    l.merge(rec(right));
                    return l; 
                }
                R sequence(Regex<T> const& left, Regex<T> const& right) const { 
                    R l(rec(left));
                    l.merge(rec(right));
                    return l; 
                }
                R kleene_star(Regex<T> const& regex) const { return rec(regex); }
                R complement(Regex<T> const& regex) const { return rec(regex); }
                R conjunction(Regex<T> const& left, Regex<T> const& right) const { 
                    R l(rec(left));
                    l.merge(rec(right));
                    return l; 
                }
            };
            template<typename T, class R> constexpr AlphabetFinder<T, R> alphabet_finder{};



            template<typename T>
            class DepthProbe: public Base<T, std::size_t> {
                using Size = std::size_t;
                using Base<T, Size>::rec;
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
            };
            template<typename T> constexpr DepthProbe<T> depth_probe{};

            template<typename T>
            class Measurer: public Base<T, std::size_t> {
                using Size = std::size_t;
                using Base<T, Size>::rec;
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
            };
            template<typename T> constexpr Measurer<T> measurer{};
        }

        
    }

    /**
     * @brief Tests whether the regex is the empty regex.
     */
    template<typename T>
    inline bool is_empty(Regex<T> const& regex) {
        return regex.match(matchers::is_empty<T>);
    }

    /**
     * @brief Tests whether the regex is the epsilon regex.
     */
    template<typename T>
    inline bool is_epsilon(Regex<T> const& regex) {
        return regex.match(matchers::is_epsilon<T>);
    }

    /**
     * @brief Tests whether the regex is the alphabet regex.
     */
    template<typename T>
    inline bool is_alphabet(Regex<T> const& regex) {
        return regex.match(matchers::is_alphabet<T>);
    }

    /**
     * @brief Tests whether the regex is the closure/repetition/(kleene)star of a regex.
     */
    template<typename T>
    inline bool is_kleene_star(Regex<T> const& regex) {
        return regex.match(matchers::is_kleene_star<T>);
    }

    /**
     * @brief Tests whether the regex is the complement of a regex.
     */
    template<typename T>
    inline bool is_complement(Regex<T> const& regex) {
        return regex.match(matchers::is_complement<T>);
    }

    /**
     * @brief Tests whether the regex is any.
     */
    template<typename T>
    inline bool is_any(Regex<T> const& regex) {
        return regex.match(matchers::is_any<T>);
    }

    /**
     * @brief Converts a regex into a `std::string`.
     *
     * The different cases are represented as follows:
     * - Empty is '∅';
     * - Epsilon is 'ε';
     * - Alphabet is 'Σ';
     * - Literals are represented using \ref Stringify;
     * - Disjunction is binary operator '|';
     * - Sequence is implicit;
     * - Kleene star is (left-)unary operator '*';
     * - Complement is (left-)unary operator '¬';
     * - Conjunction is binary operator '&'.
     *
     * The operators are bound in the following (from most to least binding) order: 
     * 1. Kleene Star/Complement;
     * 2. Sequence;
     * 3. Conjunction;
     * 4. Disjunction.
     */
    template<typename T, class Stringify = Stringify<T>>
    inline std::string to_string(Regex<T> const& regex) {
        return regex.match(matchers::printer<T, Stringify>).first;
    }

    /**
     * @brief Tests whether the regex is nullable.
     *
     * A regex is nullable iff it accepts ε (the empty string).
     */
    template<typename T, class Eq = std::equal_to<T>>
    inline bool is_nullable(Regex<T> const& regex) {
        return regex.match(matchers::nullability_checker<T, Eq>);
    }

    /**
     * @brief Derives a regex w.r.t some literal.
     * 
     * @tparam Eq Function-object defining literal equality.
     * @param x The literal to use for derivation.
     * @param regex The regex to derive.
     */
    template<typename T, class Eq = std::equal_to<T>>
    inline Regex<T> derive(T const& x, Regex<T> const& regex) {
        return regex.match(matchers::Deriver<T, Eq>{x});
    }

    /**
     * @brief Derives a regex w.r.t some literals.
     * 
     * The regex will be derive w.r.t to the first lieral
     * in the sequence, then the result will be derive w.r.t
     * the second, etc...
     *
     * @tparam Eq Function-object defining literal equality.
     * @param r The sequence of literals to use for derivation.
     * @param regex The regex to derive.
     */
    template<typename T, std::ranges::range R, class Eq = std::equal_to<T>>
    Regex<T> derive(R&& r, Regex<T> const& regex) {
        auto res = regex;
        for(
            auto beg = r.begin(), end = r.end(); 
            beg != end; 
            ++beg
        ) {
            res = derive<T, Eq>(*beg, res);
        }

        return res;
    }

    /**
     * @brief Generates the minimal alphabet on which the regex is defined.
     * 
     * The minimal alphabet is the set of all literals which explicitely
     * appear in the regex.
     *
     * @tparam R The container to use; must support `insert(T)`.
     * @param regex The regex whose alphabet must be generated.
     */
    template<typename T, class R = std::set<T>>
    R generate_minimal_alphabet(Regex<T> const& regex) {
        return regex.match(matchers::alphabet_finder<T, R>);
    }

    /**
     * @brief Computes the depth of the regex.
     * 
     * The depth of a regex is the depth of the binary tree representing it.
     *
     * @param regex The regex whose depth must be computed.
     */
    template<typename T>
    std::size_t depth(Regex<T> const& regex) {
        return regex.match(matchers::depth_probe<T>);
    }

    /**
     * @brief Computes the size of the regex.
     * 
     * The depth of a regex is the size of the binary tree representing it.
     *
     * @param regex The regex whose size must be computed.
     */
    template<typename T>
    std::size_t size(Regex<T> const& regex) {
        return regex.match(matchers::measurer<T>);
    }
}