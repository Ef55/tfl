#include "Parser.hpp"

#include "tfl/Parser.hpp"

class Parser: tfl::Parsers<Token> {
private:
    template<typename R>
    using Syntax = tfl::Parser<Token, R>;
    template<typename R>
    using Recursive = tfl::Recursive<Token, R>;


    template<typename R>
    Syntax<R> token() const {
        return elem([](auto v){ return std::holds_alternative<R>(v); })
            .map([](auto v){ return std::get<R>(v); });
    }

    template<typename R>
    Syntax<R> token(R const& r) const {
        return elem([r](auto v){ return std::holds_alternative<R>(v) && v == Token{r}; })
            .map([](auto v){ return std::get<R>(v); });
    }

    Syntax<std::string> const identifier = token<std::string>();

    Syntax<ExprAST> build_binop_parser(Syntax<ExprAST> base) const {
        std::vector<char> ops{'*', '-', '+', '<'};

        for(auto op : ops) {
            Recursive<ExprAST> cur;
            cur = (base & opt(token(op) & cur)).map([](auto p){
                auto lhs = p.first;
                auto mrhs = p.second;
                if(mrhs.has_value()){
                    auto op = mrhs.value().first;
                    auto rhs = mrhs.value().second;
                    return ExprAST::op(op, lhs, rhs);
                }
                else {
                    return lhs;
                }
            });
            base = cur;
        }

        return base;
    }

    Syntax<ExprAST> build_expr_parser() const {
        Recursive<ExprAST> any_expr;

        Syntax<ExprAST> number = token<double>().map([](auto v){ return ExprAST::number(v); });
        Syntax<ExprAST> variable = identifier.map([](auto s){ return ExprAST::variable(s); });
        Syntax<ExprAST> parenthesised = (token('(') & any_expr & token(')')).map([](auto p){ return p.first.second; });
        Syntax<ExprAST> call = (identifier & token('(') & repsep<ExprAST, char, std::vector<ExprAST>>(any_expr, token(',')) & token(')')).map(
            [](auto p){ return ExprAST::call(p.first.first.first, p.first.second); }
        );

        Syntax<ExprAST> base = number | variable | parenthesised | call;

        any_expr = build_binop_parser(base);

        return any_expr;
    }

    Syntax<ExprAST> any_expr = build_expr_parser();

    Syntax<PrototypeAST> const prototype = (identifier & token('(') & many(identifier) & token(')')).map(
        [](auto p){ return PrototypeAST(p.first.first.first, p.first.second); }
    );

    Syntax<PrototypeAST> const external = (token(Special::EXTERN) & prototype).map([](auto p){ return p.second; });

    Syntax<FunctionAST> const function = (token(Special::DEF) & prototype & any_expr).map([](auto p){
        return FunctionAST(p.first.second, p.second);
    });

    Syntax<ParsingResult> const parser = repsep1(either(any_expr, external, function), elem(';'));

public:
    ParsingResult operator()(std::vector<Token> const& input) const {
        return parser(input.cbegin(), input.cend());
    }
};

ParsingResult parse(std::vector<Token> const& input) {
    static Parser parser;
    return parser(input);
}