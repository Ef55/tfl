#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <concepts>

#include <tfl/Concepts.hpp>

class Json final {
    friend std::ostream& operator<<(std::ostream&, Json const&);

    using Object = std::vector<std::pair<std::string, Json>>;
    using Array = std::vector<Json>;
    using Variant = std::variant<Object, Array, std::string, double, bool, nullptr_t>;

    std::shared_ptr<Variant> _json;

    template<typename T> requires tfl::is_among_v<std::remove_cv_t<std::remove_reference_t<T>>, Object, Array, std::string, double, bool, nullptr_t>
    Json(T&& val): _json(std::make_shared<Variant>(std::forward<T>(val))) {}

    static void print_json(unsigned level, std::ostream& stream, Object const& obj);
    static void print_json(unsigned level, std::ostream& stream, Array const& array);
    static void print_json(unsigned level, std::ostream& stream, std::string const& str);
    static void print_json(unsigned level, std::ostream& stream, double const& d);
    static void print_json(unsigned level, std::ostream& stream, bool const& b);
    static void print_json(unsigned level, std::ostream& stream, nullptr_t const&);
    static void print_json(unsigned level, std::ostream& stream, Json const& json);

public:

    static Json null();
    static Json boolean(bool const& b);
    static Json number(double const& v);
    static Json string(std::string const& v);

    template<std::input_iterator It>
    static Json array(It beg, It end) {
        return Json(std::vector<Json>(beg, end));
    }


    static Json object();
    template<std::input_iterator It>
    static Json object(It beg, It end) {
        return Json(Object(beg, end));
    }

};

std::ostream& operator<<(std::ostream& stream, Json const& json);