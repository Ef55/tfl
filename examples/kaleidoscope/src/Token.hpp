#pragma once

#include <iostream>
#include <string>
#include <variant>

enum class Special {
    SPACE,
    DEF,
    EXTERN,
    COMMENT,
    IF,
    THEN,
    ELSE,
};

std::ostream& operator<<(std::ostream& s, Special const& t);

Special from_string(std::string const& str);

using Token = std::variant<Special, char, double, std::string>;
std::ostream& operator<<(std::ostream& s, Token const& t);