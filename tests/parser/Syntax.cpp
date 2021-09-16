#include <catch2/catch_test_macros.hpp>

#include "tfl/Syntax.hpp"

#include <functional>
#include <type_traits>

template<typename R>
using Syntax = tfl::Syntax<char, R>;
template<typename R>
using Recursive = tfl::Recursive<char, R>;
using SyntaxStructure = tfl::SyntaxStructure<char>;

#include <iostream>
class Matcher: public tfl::matchers::syntax::MutableStructureBase<char, void> {
    using tfl::matchers::syntax::MutableStructureBase<char, void>::rec;
    std::ostream& stream;

public:
    Matcher(std::ostream& s): stream(s) {}

    void elem(std::function<bool(char const&)> const& predicate) override {
        stream << 'a';
    }
    void epsilon() override {
        stream << 'e';
    }
    void disjunction(SyntaxStructure const& left, SyntaxStructure const& right) override {
        stream << '(';
        rec(left);
        stream << " | ";
        rec(right);
        stream << ')';
    }
    void sequence(SyntaxStructure const& left, SyntaxStructure const& right) override {
        stream << '(';
        rec(left);
        stream << " - ";
        rec(right);
        stream << ')';
    }
    void map(SyntaxStructure const& underlying) override {
        stream << "map(";
        rec(underlying);
        stream << ')';
    }
    void recursive(SyntaxStructure const& underlying) override {
        stream << "rec";
    }
};

TEST_CASE("Bench", "[Bench]") {
    Syntax<char> a = Syntax<char>::elem([](char c){ return c == 'a'; });
    Syntax<int> eps = Syntax<int>::eps(0);

    Recursive<int> rec;
    rec = eps | (a-rec).map([](auto p){ return p.second+1; });

    Syntax<int> s = rec;

    s.match(Matcher{std::cout});



    FAIL("fail");
}