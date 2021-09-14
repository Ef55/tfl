#pragma once

#include <functional>
#include <concepts>

namespace tfl {

    template<typename> class SyntaxStructure;

    namespace matchers {
        namespace syntax {

            template<typename T, typename R>
            class StructureBase {
            protected:
                virtual ~StructureBase() = default;

            public:
                virtual inline R rec(SyntaxStructure<T> const& syntax) const final { return syntax.match(*this); };
                virtual inline R operator()(SyntaxStructure<T> const& syntax) const final { return rec(syntax); }

                virtual R elem(std::function<bool(T const&)> const& predicate) const = 0;
                virtual R epsilon() const = 0;
                virtual R disjunction(SyntaxStructure<T> const& left, SyntaxStructure<T> const& right) const = 0;
                virtual R sequence(SyntaxStructure<T> const& left, SyntaxStructure<T> const& right) const = 0;
                virtual R map(SyntaxStructure<T> const& underlying) const = 0;
                virtual R recursive(SyntaxStructure<T> const& underlying) const = 0;
            };

            template<typename T, typename R>
            class MutableStructureBase {
            protected:
                virtual ~MutableStructureBase() = default;

            public:
                virtual inline R rec(SyntaxStructure<T> const& syntax) final { return syntax.match(*this); };
                virtual inline R operator()(SyntaxStructure<T> const& syntax) final { return rec(syntax); }

                virtual R elem(std::function<bool(T const&)> const& predicate) = 0;
                virtual R epsilon() = 0;
                virtual R disjunction(SyntaxStructure<T> const& left, SyntaxStructure<T> const& right) = 0;
                virtual R sequence(SyntaxStructure<T> const& left, SyntaxStructure<T> const& right) = 0;
                virtual R map(SyntaxStructure<T> const& underlying) = 0;
                virtual R recursive(SyntaxStructure<T> const& underlying) = 0;
            };  

        }
    }


}