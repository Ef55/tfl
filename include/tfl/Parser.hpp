#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace tfl {

    template<typename T, typename R> class Parser;

    template<typename T, typename R>
    class ParserImpl {
        friend class Parser<T, R>;
    protected:
        using It = typename std::vector<T>::const_iterator;
        using Result = std::vector<std::pair<R, It>>;

    public:
        virtual std::vector<std::pair<R, It>> apply(It const& beg, It const& end) const = 0;
    };

    template<typename T>
    class Elem: public ParserImpl<T, T> {
        std::function<bool(T const&)> const _pred;
    public:
        using Result = typename ParserImpl<T, T>::Result;
        using It = typename ParserImpl<T, T>::It;

        template<typename F>
        Elem(F&& predicate): _pred(predicate) {}

        virtual Result apply(It const& beg, It const& end) const {
            return (beg != end && _pred(*beg)) ? Result{{*beg, beg+1}} : Result{};
        }
    };

    template<typename T, typename R>
    class Epsilon: public ParserImpl<T, R> {
        std::function<R()> const _gen;
    public:
        using Result = typename ParserImpl<T, R>::Result;
        using It = typename ParserImpl<T, R>::It;

        template<typename F>
        Epsilon(F&& generator): _gen(std::forward<F>(generator)) {}

        virtual Result apply(It const& beg, It const& end) const {
            return Result{{_gen(), beg}};
        }
    };

    template<typename T, typename R>
    class Disjunction: public ParserImpl<T, R> {
        Parser<T, R> const _left;
        Parser<T, R> const _right;

    public:
        using Result = typename ParserImpl<T, R>::Result;
        using It = typename ParserImpl<T, R>::It;

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
    class Sequence: public ParserImpl<T, R> {
        Parser<T, R1> const _left;
        Parser<T, R2> const _right;

    public:
        using Result = typename ParserImpl<T, R>::Result;
        using It = typename ParserImpl<T, R>::It;

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
    class Map: public ParserImpl<T, R> {
        Parser<T, U> const _underlying;
        std::function<R(U)> const _map;

    public:
        using Result = typename ParserImpl<T, R>::Result;
        using It = typename ParserImpl<T, R>::It;

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

    // class Variable

    template<typename T, typename R>
    class Parser final {
        template<typename, typename> friend class Parser;

        using Result = typename ParserImpl<T, R>::Result;
        using It = typename ParserImpl<T, R>::It;

        std::shared_ptr<ParserImpl<T, R>> _parser;

        Parser(ParserImpl<T, R>* ptr): _parser(ptr) {}

    public:

        Result apply(It const& beg, It const& end) const {
            return _parser->apply(beg, end);
        }

        template<typename Iter>
        std::vector<R> operator()(Iter const& beg, Iter const& end) const {
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

        std::vector<R> operator()(std::initializer_list<T> ls) const {
            return operator()(ls.begin(), ls.end());
        }   



        template<
            typename F, 
            typename = std::enable_if_t<std::is_same_v<T, R>>
        >
        static Parser<T, T> elem(F&& predicate) {
            return Parser(new Elem<T>(predicate));
        }

        // template<typename F>
        // static Parser<T, T> elem(F&& predicate) {
        //     return Parser<T, T>(new Elem<T>(predicate));
        // }

        template<typename F>
        static Parser<T, R> eps(F&& generator) {
            return Parser<T, R>(new Epsilon<T, R>(generator));
        }

        Parser<T, R> operator|(Parser<T, R> const& that) const {
            return Parser<T, R>(new Disjunction<T, R>(*this, that));
        }

        template<typename R2>
        Parser<T, std::pair<R, R2>> operator&(Parser<T, R2> const& that) const {
            return Parser<T, std::pair<R, R2>>(new Sequence<T, R, R2>(*this, that));
        }

        template<typename F, typename U = std::invoke_result_t<F, R>>
        Parser<T, U> map(F&& map) const {
            return Parser<T, U>(
                new Map<T, U, R>(
                    *this, 
                    std::forward<F>(map)
                )
            );
        }
    };

    

}