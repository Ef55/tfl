#pragma once

#include <string>
#include <sstream>

#include "Concepts.hpp"

/**
 * @brief Contains the \ref tfl::Stringify class template.
 * @file
 */

namespace tfl {

    /**
     * @brief String conversion function-object.
     * 
     * Each specialization should define
     * `static std::string convert(T const& t)`.
     *
     * All types overloading `operator<<(std::ostream&, T const&)` automatically 
     * get a specialization (see \ref Stringify<Outputable>)
     *
     * @tparam T Type to convert to string.
     *
     */
    template<typename T>
    struct Stringify final {
    };

    /**
     * @brief Specialization of \ref Stringify for integral types for which std defines `to_string`.
     *
     * The types for which this specialization is enable are:
     * `int, long, long long, unsigned, unsigned long, unsigned long long, float, double, long double`
     */
    template<typename Integral> requires is_among_v<std::remove_cv_t<std::remove_reference_t<Integral>>, int, long, long long, unsigned, unsigned long, unsigned long long, float, double, long double>
    struct Stringify<Integral> final {
        static std::string convert(Integral const& i) {
            return std::to_string(i);
        }
    };

    /**
     * @brief Specialization of \ref Stringify for types overloading `operator<<(std::ostream&, T const&)`.
     */
    template<typename Outputable> requires requires(Outputable t){ std::declval<std::ostream> << t; }
    struct Stringify<Outputable> final {
        static std::string convert(Outputable const& t) {
            return (std::ostringstream{} << t).str();
        }
    };

    /**
     * @brief Specialization of \ref Stringify for `std::string`.
     */
    template<>
    struct Stringify<std::string> final {
        static std::string convert(std::string const& s) {
            return s;
        }
    };

    /**
     * @brief Specialization of \ref Stringify for `char`.
     */
    template<>
    struct Stringify<char> final {
        static std::string convert(char const& c) {
            return std::string(1, c);
        }
    };

}