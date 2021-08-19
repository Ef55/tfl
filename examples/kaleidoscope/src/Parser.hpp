#pragma once

#include <vector>

#include "Token.hpp"
#include "AST.hpp"

using ParsingResult = std::vector<std::variant<ExprAST, PrototypeAST, FunctionAST>>;

ParsingResult parse(std::vector<Token> const& input);