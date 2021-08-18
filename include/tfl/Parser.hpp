#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include <optional>
#include <variant>
#include <tuple>
#include <concepts>

namespace tfl {

    class ParsingException: public std::logic_error {
    public:
        ParsingException(std::string const& what_arg): logic_error(what_arg) {}
    };

    template<typename T, typename R> class Parser;
    template<typename T, typename R> class Recursive;
    
    class ParserImpl {
        ParserImpl() = delete;

        template<typename, typename> friend class Parser;
        template<typename, typename> friend class Recursive;

        template<typename T, typename R>
        class ParserBase {
            friend class Parser<T, R>;

        protected:
            using It = typename std::vector<T>::const_iterator;
            using Result = std::vector<std::pair<R, It>>;

        public:
            virtual ~ParserBase() = default;
            virtual std::vector<std::pair<R, It>> apply(It const& beg, It const& end) const = 0;
        };

        template<typename T>
        class Elem final: public ParserBase<T, T> {
            std::function<bool(T const&)> const _pred;

        public:
            using Result = typename ParserBase<T, T>::Result;
            using It = typename ParserBase<T, T>::It;

            template<typename F>
            Elem(F&& predicate): _pred(predicate) {}

            virtual Result apply(It const& beg, It const& end) const {
                return (beg != end && _pred(*beg)) ? Result{{*beg, beg+1}} : Result{};
            }
        };

        template<typename T, typename R>
        class Epsilon final: public ParserBase<T, R> {
            R const _val;

        public:
            using Result = typename ParserBase<T, R>::Result;
            using It = typename ParserBase<T, R>::It;

            Epsilon(R const& val): _val(val) {}

            virtual Result apply(It const& beg, It const& end) const {
                return Result{{_val, beg}};
            }
        };

        template<typename T, typename R>
        class Disjunction final: public ParserBase<T, R> {
            Parser<T, R> const _left;
            Parser<T, R> const _right;

        public:
            using Result = typename ParserBase<T, R>::Result;
            using It = typename ParserBase<T, R>::It;

            Disjunction(Parser<T, R> const& left, Parser<T, R> const& right): _left(left), _right(right) {}

            virtual Result apply(It const& beg, It const& end) const {
                Result l(_left.apply(beg, end));
                Result r(_right.apply(beg, end));
                l.reserve(l.size() + r.size());
                l.insert(l.end(), r.begin(), r.end());
                return l;
            }
        };

        template<typename T, typename R1, typename R2, typename R = std::pair<R1, R2>>
        class Sequence final: public ParserBase<T, R> {
            Parser<T, R1> const _left;
            Parser<T, R2> const _right;

        public:
            using Result = typename ParserBase<T, R>::Result;
            using It = typename ParserBase<T, R>::It;

            Sequence(Parser<T, R1> const& left, Parser<T, R2> const& right): _left(left), _right(right) {}

            virtual Result apply(It const& beg, It const& end) const {
                Result res;
                auto l(_left.apply(beg, end));

                for(auto& p : l) {
                    auto follow(_right.apply(p.second, end));

                    res.reserve(res.size() + follow.size());
                    for(auto& f : follow) {
                        res.emplace_back(R{p.first, f.first}, f.second);
                    }
                }

                return res;
            }
        };

        template<typename T, typename R, typename U>
        class Map final: public ParserBase<T, R> {
            Parser<T, U> const _underlying;
            std::function<R(U)> const _map;

        public:
            using Result = typename ParserBase<T, R>::Result;
            using It = typename ParserBase<T, R>::It;

            template<typename F>
            Map(Parser<T, U> const& underlying, F&& map): _underlying(underlying), _map(map) {}

            virtual Result apply(It const& beg, It const& end) const {
                Result res;
                auto sub(_underlying.apply(beg, end));
                res.reserve(res.size());

                for(auto& p : sub) {
                    res.emplace_back(_map(p.first), p.second);
                }

                return res;
            }
        };

        template<typename T, typename R>
        class Recursion final: public ParserBase<T, R> {
            std::weak_ptr<ParserBase<T, R>> _rec;

        public:
            using Result = typename ParserBase<T, R>::Result;
            using It = typename ParserBase<T, R>::It;

            Recursion(): _rec() {}

            void init(std::shared_ptr<ParserBase<T, R>> const& ptr) {
                _rec = ptr;
            }

            virtual Result apply(It const& beg, It const& end) const {
                auto p = _rec.lock();
                if(!p) {
                    throw ParsingException("Parser expired.");
                }
                auto r = p->apply(beg, end);
                return r;
            }
        };
    };


    template<typename T, typename R>
    class Parser final {
        template<typename, typename> friend class Parser;
        template<typename, typename> friend class Recursive;

        using Result = typename ParserImpl::ParserBase<T, R>::Result;
        using It = typename ParserImpl::ParserBase<T, R>::It;

        std::shared_ptr<ParserImpl::ParserBase<T, R>> _parser;

        Parser(ParserImpl::ParserBase<T, R>* ptr): _parser(ptr) {}
        Parser(std::shared_ptr<ParserImpl::ParserBase<T, R>> ptr): _parser(ptr) {}

    public:
        using TokenType = T;
        using ValueType = R;

        Result apply(It const& beg, It const& end) const {
            return _parser->apply(beg, end);
        }

        template<typename Iter>
        R operator()(Iter const& beg, Iter const& end) const {
            auto r = parser_all(beg, end);

            if(r.size() != 1) {
                throw ParsingException("Parsing failed: " + std::to_string(r.size()) + " match(es).");
            }
            else {
                return r[0];
            }
        }

        R operator()(std::initializer_list<T> ls) const {
            return operator()(ls.begin(), ls.end());
        }   

        template<typename Iter>
        std::vector<R> parser_all(Iter const& beg, Iter const& end) const {
            std::vector<T> in(beg, end);
            Result p{apply(in.begin(), in.end())};

            std::vector<R> res;
            for(auto& r : p) {
                if(r.second == in.end()) {
                    res.push_back(r.first);
                }
            }

            return res;
        }

        std::vector<R> parser_all(std::initializer_list<T> ls) const {
            return operator()(ls.begin(), ls.end());
        } 

        template<typename F> requires std::same_as<T, R>
        static Parser<T, T> elem(F&& predicate) {
            return Parser<T, T>(new ParserImpl::Elem<T>(predicate));
        }

        static Parser<T, R> eps(R const& val) {
            return Parser<T, R>(new ParserImpl::Epsilon<T, R>(val));
        }

        Parser<T, R> operator|(Parser<T, R> const& that) const {
            return Parser<T, R>(new ParserImpl::Disjunction<T, R>(*this, that));
        }

        template<typename R2>
        Parser<T, std::pair<R, R2>> operator&(Parser<T, R2> const& that) const {
            return Parser<T, std::pair<R, R2>>(new ParserImpl::Sequence<T, R, R2>(*this, that));
        }

        template<typename F, typename U = std::invoke_result_t<F, R>>
        Parser<T, U> map(F&& map) const {
            return Parser<T, U>(
                new ParserImpl::Map<T, U, R>(
                    *this, 
                    std::forward<F>(map)
                )
            );
        }

        Parser<T, R> operator|(Recursive<T, R> const& that) const {
            return operator|(static_cast<Parser<T, R>>(that));
        }

        template<typename R2>
        Parser<T, std::pair<R, R2>> operator&(Recursive<T, R2> const& that) const {
            return operator&(static_cast<Parser<T, R2>>(that));
        }
    };

    template<typename T, typename R>
    class Recursive final {
        std::shared_ptr<ParserImpl::Recursion<T, R>> _rec;
        std::optional<Parser<T, R>> _init;

    public:
        Recursive(): _rec(new ParserImpl::Recursion<T, R>()), _init(std::nullopt) {}
        Recursive(Recursive const&) = delete;
        Recursive& operator=(Recursive const&) = delete;

        operator Parser<T, R> () const {
            return _init ? _init.value() : Parser<T, R>(_rec);
        }

        Parser<T, R> operator=(Parser<T, R> const& that) {
            if(_init){
                throw ParsingException("Recursive already defined");
            }
            _rec->init(that._parser);
            _init = that;
            return that;
        }

        Parser<T, R> operator|(Parser<T, R> const& that) const {
            return static_cast<Parser<T, R>>(*this) | that;
        }

        template<typename R2>
        Parser<T, std::pair<R, R2>> operator&(Parser<T, R2> const& that) const {
            return static_cast<Parser<T, R>>(*this) & that;
        }

        template<typename F, typename U = std::invoke_result_t<F, R>>
        Parser<T, U> map(F&& map) {
            return static_cast<Parser<T, R>>(*this).map(std::forward<F>(map));
        }
    };

    template<typename T>
    struct Parsers {
    private:

        static constexpr auto right_pushback_left = [](auto p){ p.second.push_back(p.first); return p.second; };
        static constexpr auto reverse = [](auto ls){ std::reverse(ls.begin(), ls.end()); return ls; };
        static constexpr auto drop_left = [](auto p){ return p.second; };

        template<typename W, typename E>
        static constexpr auto wrap = [](E&& e){ return W{std::forward<E>(e)}; };

    public:
        template<std::predicate<T> F>
        static Parser<T, T> elem(F&& predicate) {
            return Parser<T, T>::elem(std::forward<F>(predicate));
        }

        static Parser<T, T> elem(T const& val) {
            return Parser<T, T>::elem([val](T const& i){ return i == val; });
        }

        static Parser<T, T> success() {
            return Parser<T, T>::elem([](T const& i){ return true; });
        }

        static Parser<T, T> failure() {
            return Parser<T, T>::elem([](T const& i){ return false; });
        }

        template<typename R>
        static Parser<T, R> eps(R const& val) {
            return Parser<T, R>::eps(val);
        }

        template<typename R>
        static Recursive<T, R> recursive() {
            return Recursive<T, R>();
        }

        template<typename R>
        static Parser<T, std::optional<R>> opt(Parser<T, R> p) {
            return eps(std::optional<R>(std::nullopt)) | p.map(wrap<std::optional<R>, R>);
        }

        template<
            typename R,
            typename Result = std::vector<R>
        >
        static Parser<T, Result> many(Parser<T, R> const& elem) {
            Recursive<T, Result> rec;
            rec = 
                eps(Result{}) |
                (elem & rec)
                    .map(right_pushback_left);

            return rec.map(reverse);
        }

        template<
            typename R,
            typename Result = std::vector<R>
        >
        static Parser<T, Result> many1(Parser<T, R> const& elem) {
            Recursive<T, Result> rec;
            rec = (
                    elem & 
                    (eps(Result{}) | rec)
                ).map(right_pushback_left);

            return rec.map(reverse);
        }

        template<
            typename R,
            typename S,
            typename Result = std::vector<R>
        >
        static Parser<T, Result> repsep1(Parser<T, R> const& elem, Parser<T, S> const& sep) {
            Recursive<T, Result> rec;
            rec = 
                eps(Result{}) |
                (
                    (sep & elem).map(drop_left)
                    & rec
                ).map(right_pushback_left);

            return (elem & rec).map(right_pushback_left).map(reverse);
        }

        template<
            typename R,
            typename S,
            typename Result = std::vector<R>
        >
        static Parser<T, Result> repsep(Parser<T, R> const& elem, Parser<T, S> const& sep) {
            return eps(Result{}) | repsep1(elem, sep);
        }

        template<typename R, typename... Types>
        static Parser<T, std::variant<R, Types...>> either(Parser<T, R> const& p, Parser<T, Types>... others) {
            return (p.map(wrap<std::variant<R, Types...>, R>) | ... | others.map(wrap<std::variant<R, Types...>, Types>));
        }

        // template<typename R, typename... Types>
        // static Parser<T, std::tuple<R, Types...>> sequence(Parser<T, R> const& p, Parser<T, Types>... others) {}


    protected:
        Parsers() {}
    };
    
}