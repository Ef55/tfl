#pragma once

#include <vector>

#include "Token.hpp"
#include "Json.hpp"

Json parse(std::vector<Token> const& input);