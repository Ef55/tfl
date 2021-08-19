#include "Token.hpp"

#include <stdexcept>

std::ostream& operator<<(std::ostream& s, Special const& t) {
    switch(t) {
        using enum Special;
        case SPACE: return s << ' ';
        case DEF: return s << "def";
        case EXTERN: return s << "extern";
        case COMMENT: return s << "\n#...\n";
        case IF: return s << "if";
        case THEN: return s << "then";
        case ELSE: return s << "else";
        default: throw std::logic_error("Invalid special: " + static_cast<int>(t));
    }
}

Special from_string(std::string const& str) {
    if(str == "def") {
        return Special::DEF;
    }
    else if(str == "extern") {
        return Special::EXTERN;
    }
    else if(str == "if") {
        return Special::IF;
    }
    else if(str == "then") {
        return Special::THEN;
    }
    else if(str == "else") {
        return Special::ELSE;
    }
    else {
        throw std::logic_error("Impossible special: `" + str + "`");
    }
}

std::ostream& operator<<(std::ostream& s, Token const& t) {
    std::visit([&s](auto v){ s << v; }, t);
    return s;
}