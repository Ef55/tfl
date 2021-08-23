#pragma once

#include <string>
#include <sstream>

#include "Concepts.hpp"

namespace tfl {

    template<typename T>
    struct Stringify final {

        std::string operator()(T const& t) const {
            return (std::ostringstream{} << t).str();
        }
    };

    template<>
    struct Stringify<std::string> final {

        std::string operator()(std::string const& s) const {
            return s;
        }
    };

    template<>
    struct Stringify<char> final {

        std::string operator()(char const& c) const {
            return std::string(1, c);
        }
    };

    template<typename T> requires is_among_v<std::remove_cv_t<std::remove_reference_t<T>>, int, long, long long, unsigned, unsigned long, unsigned long long, float, double, long double>
    struct Stringify<T> final {

        std::string operator()(T const& t) const {
            return std::to_string(t);
        }
    };

}