#pragma once

#include <string>
#include <sstream>

#include "Concepts.hpp"

namespace tfl {

    template<typename T>
    struct Stringify final {
        static std::string convert(T const& t) {
            return (std::ostringstream{} << t).str();
        }
    };

    template<>
    struct Stringify<std::string> final {
        static std::string convert(std::string const& s) {
            return s;
        }
    };

    template<>
    struct Stringify<char> final {
        static std::string convert(char const& c) {
            return std::string(1, c);
        }
    };

    template<typename T> requires is_among_v<std::remove_cv_t<std::remove_reference_t<T>>, int, long, long long, unsigned, unsigned long, unsigned long long, float, double, long double>
    struct Stringify<T> final {
        static std::string convert(T const& t) {
            return std::to_string(t);
        }
    };
}