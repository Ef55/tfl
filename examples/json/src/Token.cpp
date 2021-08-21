#include "Token.hpp"

std::ostream& operator<<(std::ostream& stream, Token const& t) {
    std::visit([&stream](auto v){ stream << v; }, t);
    return stream;
}