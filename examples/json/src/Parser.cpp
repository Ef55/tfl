#include "Parser.hpp"

#include <stdexcept>
#include "tfl/Parser.hpp"

class: tfl::Parsers<Token> {
private:
    template<typename R>
    using Parser = tfl::Parser<Token, R>;
    template<typename R>
    using Recursive = tfl::Recursive<Token, R>;

    template<typename R>
    Parser<R> token() const {
        return elem([](auto v){ return std::holds_alternative<R>(v); })
            .map([](auto v){ return std::get<R>(v); });
    }

    template<typename R>
    Parser<R> token(R const& r) const {
        return elem([r](auto v){ return std::holds_alternative<R>(v) && v == Token{r}; })
            .map([](auto v){ return std::get<R>(v); });
    }

    Parser<Json> build_value_parser() const {
        Recursive<Json> value;

        auto whitespace = opt(token(' '));

        Parser<std::string> str = token<std::string>();
        Parser<char> oobj = token('{');
        Parser<char> cobj = token('}');
        Parser<char> oarr = token('[');
        Parser<char> carr = token(']');
        Parser<char> sep = token(',');

        Parser<Json> boolean = token<bool>().map([](auto b){ return Json::boolean(b); });
        Parser<Json> null = token<nullptr_t>().map([](auto){ return Json::null(); });
        Parser<Json> number = token<double>().map([](auto d){ return Json::number(d); });
        Parser<Json> string = str.map([](auto s){ return Json::string(s); });

        Parser<Json> array = (oarr & repsep(static_cast<Parser<Json>>(value), sep) & carr).map(
            [](auto p){ 
                auto v = p.first.second; 
                return Json::array(v.cbegin(), v.cend());
            }
        );

        auto key_sep = whitespace & token(':');
        Parser<std::pair<std::string, Json>> key_val = (whitespace & str & key_sep & value).map(
            [](auto p){ return std::pair{p.first.first.second, p.second}; }
        );

        Parser<Json> object_body = 
            whitespace.map(
                [](auto){ return Json::object(); }
            ) | 
            repsep1(key_val, sep).map(
                [](auto v){ return Json::object(v.cbegin(), v.cend()); }
            );

        Parser<Json> object = (oobj & object_body & cobj).map(
            [](auto p){ return p.first.second; }
        );

        value = (whitespace & (boolean | null | number | string | array | object) & whitespace).map(
            [](auto p){ return p.first.second; }
        );

        return value;
    }

    Parser<Json> const parser = build_value_parser();

public:
    Json operator()(std::vector<Token> const& input) const {
        return parser(input.cbegin(), input.cend());
    }
} parser;

Json parse(std::vector<Token> const& input) {
    return parser(input);
}