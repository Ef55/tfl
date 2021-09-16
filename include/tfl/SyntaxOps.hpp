#pragma once

#include <functional>
#include <concepts>
#include <unordered_set>

#include <iostream>
namespace tfl {
    class RecId final {
        friend struct std::hash<RecId>;
        template<typename> friend class SyntaxStructure;
        void* _ptr;

        RecId(void* ptr): _ptr(ptr) {}

    public:
        bool operator==(RecId const& that) const {
            return _ptr == that._ptr;
        }
    };
}

namespace std {
    template<>
    struct hash<tfl::RecId> {
        size_t operator()(tfl::RecId const& id) const {
            return std::hash<void*>{}(id._ptr);
        }
    };
}

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
                virtual R recursive(SyntaxStructure<T> const& underlying, RecId const& id) const = 0;
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
                virtual R recursive(SyntaxStructure<T> const& underlying, RecId const& id) = 0;
            };  

            template<typename T>
            class RecursionMemoizer: public MutableStructureBase<T, void> {
            private:
                std::unordered_set<RecId> _ids;

            protected:
                virtual void new_recursive(SyntaxStructure<T> const& underlying, RecId const& id) = 0;

            public:
                virtual void recursive(SyntaxStructure<T> const& underlying, RecId const& id) final override {
                    if(!_ids.contains(id)) {
                        _ids.insert(id);
                        new_recursive(underlying, id);
                    }
                }
            };
        }
    }


}
