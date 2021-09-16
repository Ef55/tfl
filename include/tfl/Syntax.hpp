#pragma once

#include <functional>
#include <memory>
#include <variant>

#include "Lazy.hpp"
#include "Concepts.hpp"
#include "SyntaxOps.hpp"

namespace tfl {

    template<typename, typename> class Syntax;


    template<typename T>
    class SyntaxStructure final {
        template<typename> friend class Elem;
        template<typename, typename> friend class Epsilon;
        template<typename, typename> friend class Disjunction;
        template<typename, typename, typename> friend class Sequence;
        template<typename, typename, typename> friend class Map;
        template<typename, typename> friend class RecursiveInter;

        struct Elem final {
            std::function<bool(T const&)> const _predicate;

            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.elem(_predicate); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.elem(_predicate); }
        };

        struct Epsilon final {
            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.epsilon(); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.epsilon(); }
        };

        struct Disjunction final {
            Lazy<SyntaxStructure<T>> const _left;
            Lazy<SyntaxStructure<T>> const _right;

            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.disjunction(_left, _right); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.disjunction(_left, _right); }
        };

        struct Sequence final {
            Lazy<SyntaxStructure<T>> const _left;
            Lazy<SyntaxStructure<T>> const _right;

            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.sequence(_left, _right); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.sequence(_left, _right); }
        };

        struct Map final {
            Lazy<SyntaxStructure<T>> const _underlying;

            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.map(_underlying); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.map(_underlying); }
        };

        struct Recursive final {
            using Var = std::variant<
                Elem,
                Epsilon,
                Disjunction,
                Sequence,
                Map,
                Recursive
            >;
            std::weak_ptr<Var> const _underlying;

            Recursive() = delete;
            Recursive(SyntaxStructure<T> const str): _underlying(str._syntax) {}

            template<typename MR>
            MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const { return matcher.recursive(SyntaxStructure<T>(_underlying.lock())); }
            template<typename MR>
            MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const { return matcher.recursive(SyntaxStructure<T>(_underlying.lock())); }
        };

        using Var = std::variant<
            Elem,
            Epsilon,
            Disjunction,
            Sequence,
            Map,
            Recursive
        >;
        std::shared_ptr<Var> _syntax;

        template<typename V> requires is_among_v<V, Elem, Epsilon, Disjunction, Sequence, Map, Recursive>
        SyntaxStructure(V&& val): _syntax(std::make_shared<Var>(std::forward<V>(val))) {}

        SyntaxStructure(std::shared_ptr<Var> ptr): _syntax(ptr) {}

    public:
        template<typename MR>
        MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_syntax);
        }

        template<typename MR>
        MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_syntax);
        }

        template<typename MR>
        MR match(matchers::syntax::MutableStructureBase<T, MR>&& matcher) const {
            return std::visit([&matcher](auto r){ return r.match(matcher); }, *_syntax);
        }
    };

    namespace {
        template<typename T, typename R>
        struct SyntaxBase {
            Lazy<SyntaxStructure<T>> _structure;
            SyntaxBase(): _structure(Lazy<SyntaxStructure<T>>::computation([this](){ return this->generate_structure(); })) {}
            virtual ~SyntaxBase() = default;
        
            virtual SyntaxStructure<T> generate_structure() const = 0;
            virtual SyntaxStructure<T> structure() const final {
                return SyntaxStructure<T>(_structure.get());
            }
        };

        template<typename T>
        struct Elem final: public SyntaxBase<T, T> {
            std::function<bool(T const&)> const _predicate;

            template<typename F>
            Elem(F&& predicate): _predicate(std::forward<F>(predicate)) {}

            SyntaxStructure<T> generate_structure() const override {
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Elem(_predicate));
            }
        };

        template<typename T, typename R>
        struct Epsilon final: public SyntaxBase<T, R> {
            R const _val;
            Epsilon(R const& val): _val(val) {}

            SyntaxStructure<T> generate_structure() const override {
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Epsilon());
            }
        };

        template<typename T, typename R>
        struct Disjunction final: public SyntaxBase<T, R> {
            Syntax<T, R> const _left;
            Syntax<T, R> const _right;
            Disjunction(Syntax<T, R> const& l, Syntax<T, R> const& r): _left(l), _right(r) {}

            SyntaxStructure<T> generate_structure() const override {
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Disjunction(
                    Lazy<SyntaxStructure<T>>::computation([*this](){ return _left.structure(); }), 
                    Lazy<SyntaxStructure<T>>::computation([*this](){ return _right.structure(); })
                ));
            }
        };

        template<typename T, typename L, typename R>
        struct Sequence final: public SyntaxBase<T, std::pair<L, R>> {
            Syntax<T, L> const _left;
            Syntax<T, R> const _right;
            Sequence(Syntax<T, L> const& l, Syntax<T, R> const& r): _left(l), _right(r) {}

            SyntaxStructure<T> generate_structure() const override {
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Sequence(
                    Lazy<SyntaxStructure<T>>::computation([*this](){ return _left.structure(); }), 
                    Lazy<SyntaxStructure<T>>::computation([*this](){ return _right.structure(); })
                ));
            }
        };

        template<typename T, typename U, typename R>
        struct Map final: public SyntaxBase<T, R> {
            Syntax<T, U> const _underlying;
            std::function<R(U)> const _map;
            template<typename F>
            Map(Syntax<T, U> const& u, F&& map): _underlying(u), _map(std::forward<F>(map)) {}

            SyntaxStructure<T> generate_structure() const override {
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Map(
                    Lazy<SyntaxStructure<T>>::computation([*this](){ return _underlying.structure(); })
                ));
            }
        };

        template<typename T, typename R>
        struct RecursiveInter final: public SyntaxBase<T, R> {
            std::weak_ptr<SyntaxBase<T, R>> _rec;
            RecursiveInter(): _rec() {}

            void init(std::shared_ptr<SyntaxBase<T, R>> const& ptr) {
                _rec = ptr;
            }

            SyntaxStructure<T> generate_structure() const override {
                auto p = _rec.lock();
                if(!p) {
                    throw std::logic_error("Cannot generate generate_structure of an uninitialized recursive syntax.");
                }

                // Stop gap to avoid inifinite recursion
                // TODO: restore
                //return SyntaxStructure<T>(typename SyntaxStructure<T>::Epsilon());
                return SyntaxStructure<T>(typename SyntaxStructure<T>::Recursive(p->structure()));
            }
        };
    }

    template<typename T, typename R>
    class Recursive final {
        std::shared_ptr<RecursiveInter<T, R>> _rec;
        std::optional<Syntax<T, R>> _init;

    public:
        Recursive(): _rec(std::make_shared<RecursiveInter<T, R>>()), _init(std::nullopt) {}
        Recursive(Recursive const&) = delete;
        Recursive& operator=(Recursive const&) = delete;

        operator Syntax<T, R> () const {
            return _init ? _init.value() : Syntax<T, R>(_rec);
        }

        Syntax<T, R> operator=(Syntax<T, R> const& that) {
            if(_init){
                throw std::logic_error("Recursive already defined");
            }
            _rec->init(that._syntax);
            _init = that;
            return that;
        }

        Syntax<T, R> operator|(Syntax<T, R> const& that) const {
            return static_cast<Syntax<T, R>>(*this) | that;
        }

        template<typename R2>
        Syntax<T, std::pair<R, R2>> operator-(Syntax<T, R2> const& that) const {
            return static_cast<Syntax<T, R>>(*this) & that;
        }

        template<std::invocable<R> F>
        Syntax<T, std::invoke_result_t<F, R>> map(F&& map) {
            return static_cast<Syntax<T, std::invoke_result_t<F, R>>>(*this).map(std::forward<F>(map));
        }
    };

    template<typename T, typename R>
    class Syntax final {
        template<typename, typename> friend class Syntax;
        template<typename, typename> friend class Recursive;

        std::shared_ptr<SyntaxBase<T, R>> _syntax;

        Syntax(std::shared_ptr<SyntaxBase<T, R>> const& ptr): _syntax(ptr) {}
    public:
        SyntaxStructure<T> structure() const {
            return _syntax->structure();
        }

        template<typename MR>
        MR match(matchers::syntax::StructureBase<T, MR> const& matcher) const {
            return structure().match(matcher);
        }

        template<typename MR>
        MR match(matchers::syntax::MutableStructureBase<T, MR>& matcher) const {
            return structure().match(matcher);
        }

        template<typename MR>
        MR match(matchers::syntax::MutableStructureBase<T, MR>&& matcher) const {
            return structure().match(matcher);
        }




        template<std::predicate<T> F> requires std::same_as<T, R>
        static Syntax<T, T> elem(F&& predicate) {
            return Syntax<T, T>(std::make_shared<Elem<T>>(std::forward<F>(predicate)));
        }

        static Syntax<T, R> eps(R const& val) {
            return Syntax<T, R>(std::make_shared<Epsilon<T, R>>(val));
        }

        Syntax<T, R> operator|(Syntax<T, R> const& that) const {
            return Syntax<T, R>(std::make_shared<Disjunction<T, R>>(*this, that));
        }

        template<typename R2>
        Syntax<T, std::pair<R, R2>> operator-(Syntax<T, R2> const& that) const {
            return Syntax<T, std::pair<R, R2>>(std::make_shared<Sequence<T, R, R2>>(*this, that));
        }

        template<std::invocable<R> F>
        Syntax<T, std::invoke_result_t<F, R>> map(F&& map) const {
            using Rn = std::invoke_result_t<F, R>;
            return Syntax<T, Rn>(
                std::make_shared<Map<T, R, Rn>>(
                    *this, 
                    std::forward<F>(map)
                )
            );
        }

        Syntax<T, R> operator|(Recursive<T, R> const& that) const {
            return operator|(static_cast<Syntax<T, R>>(that));
        }

        template<typename R2>
        Syntax<T, std::pair<R, R2>> operator-(Recursive<T, R2> const& that) const {
            return operator-(static_cast<Syntax<T, R2>>(that));
        }
    };
}