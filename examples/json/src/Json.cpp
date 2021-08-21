#include "Json.hpp"

void Json::print_json(unsigned level, std::ostream& stream, Object const& obj) {
    auto tab = std::string(level+1, '\t');

    stream << "{\n";
    for(auto elem: obj) {
        stream << tab;
        print_json(level+1, stream, elem.first);
        stream << ": ";
        print_json(level+1, stream, elem.second);
        stream << ",\n";
    }
    stream << std::string(level, '\t') << "}";
}

void Json::print_json(unsigned level, std::ostream& stream, Array const& array) {
    auto tab = std::string(level+1, '\t');

    stream << "[\n";
    for(auto elem: array) {
        stream << tab;
        print_json(level+1, stream, elem);
        stream << ",\n";
    }
    stream << std::string(level, '\t') << "]";
}

void Json::print_json(unsigned level, std::ostream& stream, std::string const& str) {
    stream << '"' << str << '"';
}

void Json::print_json(unsigned level, std::ostream& stream, double const& d) {
    stream << d;
}

void Json::print_json(unsigned level, std::ostream& stream, bool const& b) {
    stream << (b ? "true" : "false");
}

void Json::print_json(unsigned level, std::ostream& stream, nullptr_t const&) {
    stream << "null";
}

void Json::print_json(unsigned level, std::ostream& stream, Json const& json) {
    std::visit([&level, &stream](auto const& v){ print_json(level, stream, v); }, *json._json);
}


Json Json::null() {
    return Json(nullptr);
}

Json Json::boolean(bool const& b) {
    return Json(b);
}

Json Json::number(double const& v) {
    return Json(v);
}

Json Json::string(std::string const& v) {
    return Json(v);
}

Json Json::object() {
    return Json(Object());
}

std::ostream& operator<<(std::ostream& stream, Json const& json) {
    Json::print_json(0, stream, json);
    return stream;
}