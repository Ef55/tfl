#pragma once

#include <iostream>
#include <string>
#include <variant>

using Token = std::variant<char, bool, nullptr_t, double, std::string>;

std::ostream& operator<<(std::ostream& stream, Token const& t);