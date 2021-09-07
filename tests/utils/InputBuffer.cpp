#include <catch2/catch_test_macros.hpp>

#include "tfl/InputBuffer.hpp"

#include <memory>
#include <sstream>
#include <ranges>
#include <iterator>
#include <type_traits>

#include <iostream>

namespace {
    using namespace std::ranges;

    template<typename T>
    using InputBuffer = tfl::InputBuffer<T>;
}

class Integers {
    std::shared_ptr<int> _last = std::make_shared<int>(0);
public:
    using difference_type = int;

    int last() const {
        return *_last;
    }

    Integers& operator++() {
        ++(*_last);
        return *this;
    }

    int operator++(int) {
        return (*_last)++;
    }

    bool operator==(int i) const {
        return *_last == i;
    }
};

namespace Catch {
    template<>
    struct StringMaker<Integers> {
        static std::string convert(Integers const& value) {
            return "Integers{" + std::to_string(value.last()) + '}';
        }
    };
}

TEST_CASE("Input buffer satisfies input_range and output_range", "[input-buffer]") {
    using T = char;
    using B = InputBuffer<empty_view<char>>;
    using It = B::Iterator;

    STATIC_REQUIRE( std::is_same_v<B::ValueType, T> );

    STATIC_REQUIRE( std::input_or_output_iterator<It> );
    STATIC_REQUIRE( std::input_iterator<It> );
    STATIC_REQUIRE( std::output_iterator<It, T> );
    STATIC_REQUIRE( std::sentinel_for<B::Sentinel, It> );

    STATIC_REQUIRE( range<B> );
    STATIC_REQUIRE( input_range<B> );
    STATIC_REQUIRE( output_range<B, T> );
}

TEST_CASE("Iterator operators behave as expected", "[input-buffer]") {
    InputBuffer buf(iota_view(0, 3));


    SECTION("Increment and dereference") {
        auto it = std::ranges::begin(buf);

        REQUIRE( *it == 0 );
        REQUIRE( *it++ == 0 );
        REQUIRE( *it == 1 );
        REQUIRE( *++it == 2 );
        *it = 3;
        REQUIRE( *it == 3 );
        ++it;
        REQUIRE_THROWS_AS( *it, std::out_of_range );
    }

    SECTION("Difference and ordering") {
        InputBuffer other_buf(iota_view(0, 4));
        auto it = std::ranges::begin(buf);

        REQUIRE( it == it );
        REQUIRE( (it <=> other_buf.begin()) == std::partial_ordering::unordered );

        for(int i = 0; i < 4; ++i) {
            
            REQUIRE( (it - buf.begin()) == i );
            REQUIRE( (buf.begin() - it) == -i );

            ++it;
            REQUIRE( (it <=> other_buf.begin()) == std::partial_ordering::unordered );
            REQUIRE( buf.begin() < it );
        }
    }
}

TEST_CASE("Buffer access", "[input-buffer]") {
    Integers i;

    InputBuffer buf{views::transform(
        iota_view(i),
        [](auto i){ return i.last(); }
    )};

    SECTION("Input is consumed lazily") {
        REQUIRE( i == 0 );
        REQUIRE( buf.buffed_size() == 0 );
        REQUIRE( !buf.consumed_all() );

        REQUIRE( buf[2] == 2 );
        REQUIRE( i == 3 );
        REQUIRE( buf.buffed_size() == 3 );
        REQUIRE( !buf.consumed_all() );
    }

    SECTION("Data can be accessed again") {
        REQUIRE( buf.buffed_size() == 0 );

        for(auto i = 0; i < 3; ++i) {
            REQUIRE( buf[i] == i );
        }

        REQUIRE( i == 3 );
        REQUIRE( buf.buffed_size() == 3 );

        for(auto i = 0; i < 3; ++i) {
            REQUIRE( buf[i] == i );
        }

        REQUIRE( i == 3 );
        REQUIRE( buf.buffed_size() == 3 );
    }
    

    SECTION("Data can be released") {
        REQUIRE( buf.buffed_size() == 0 );

        for(auto i = 0; i < 3; ++i) {
            REQUIRE( buf[i] == i );
        }

        buf.release(2);
        REQUIRE( buf.buffed_size() == 1 );
        REQUIRE( buf[0] == 2 );
        REQUIRE( i == 3 );
        REQUIRE( !buf.consumed_all() );

        REQUIRE_THROWS_AS( buf.release(2), std::invalid_argument );
        REQUIRE( buf.buffed_size() == 0 );

        REQUIRE( buf[0] == 3 );
        REQUIRE( i == 4 );
    }
    
}

TEST_CASE("Input buffer can be used in for-range", "[input-buffer]") {
    std::istringstream stream("0 1 2");

    auto range = subrange((std::istream_iterator<int>(stream)), (std::istream_iterator<int>()));
    InputBuffer buffer(range);

    int i = 0;
    for(int j: buffer) {
        REQUIRE( i == j );
        ++i;
    }
    REQUIRE( i == 3 );
}