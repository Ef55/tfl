#include <catch2/catch_test_macros.hpp>

#include "tfl/Lazy.hpp"

#include <vector>

namespace{
    template<typename T>
    using Lazy = tfl::Lazy<T>;
}

TEST_CASE("Lazy can be built as expected", "[lazy]") {
    Lazy<double> cv = Lazy<double>::value(int(5));
    REQUIRE( cv.get() == 5.0 );

    Lazy<double> cc = Lazy<double>::computation([](){ return 6; });
    REQUIRE( cc.get() == 6.0 );

    Lazy<double> acc = Lazy<double>::computation([](int x){ return x; }, 7);
    REQUIRE( acc.get() == 7.0 );

    Lazy<std::vector<char>> cstr = Lazy<std::vector<char>>::construction(3, 'a');
    REQUIRE( cstr.get() == std::vector<char>{ 'a', 'a', 'a' } );
}

TEST_CASE("Lazy evaluates lazily", "[lazy]") {
    int eval_count = 0;
    Lazy<int> lazy = Lazy<int>::computation(
        [&eval_count](){ ++eval_count; return 0; }
    );

    REQUIRE( eval_count == 0 );
    REQUIRE( lazy.evaluated() == false );

    SECTION("kick forces evaluation") {
        lazy.get();

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );
    }
    
    SECTION("get kicks evaluation") {
        REQUIRE( lazy.get() == 0 );

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );
    }


    SECTION("evaluation is not triggered multiple times") {
        // First
        lazy.kick();

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );

        // Second
        REQUIRE( lazy.get() == 0 );

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );

        // Third
        REQUIRE( lazy.get() == 0 );

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );


        // Fourth
        lazy.kick();

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );
    }

    SECTION("evaluation is not triggered multiple times even when copied") {
        auto lazy2 = lazy;

        // First
        lazy.kick();

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );

        // Second
        REQUIRE( lazy2.get() == 0 );

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy2.evaluated() == true );

        // Third
        REQUIRE( lazy.get() == 0 );

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy.evaluated() == true );


        // Fourth
        lazy2.kick();

        REQUIRE( eval_count == 1 );
        REQUIRE( lazy2.evaluated() == true );
    }
}

TEST_CASE("Lazy is a monad", "[lazy]") {
    int eval_count = 0;
    Lazy<char> lazy = Lazy<char>::computation(
        [&eval_count](){ ++eval_count; return '*'; }
    );

    REQUIRE( eval_count == 0 );
    REQUIRE( lazy.evaluated() == false );

    SECTION("Map") {

        SECTION("Correctly mapped") {
            auto mlazy = lazy.map([](char c){ return std::vector<char>(3, c); });

            REQUIRE( mlazy.get() == std::vector<char>{ '*', '*', '*' } );
        }

        SECTION("Evaluation orders") {
            auto mlazy = lazy.map([](char c) ->int { return c; });

            REQUIRE( eval_count == 0 );
            REQUIRE( lazy.evaluated() == false );
            REQUIRE( mlazy.evaluated() == false );

            SECTION("Map evaluated before base") {
                REQUIRE( mlazy.get() == 42 );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );

                REQUIRE( lazy.get() == '*' );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );
            }

            SECTION("Base evaluated before map") {
                REQUIRE( lazy.get() == '*' );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == false );

                REQUIRE( mlazy.get() == 42 );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );
            }
        }

        SECTION("Map can outlive base") {
            Lazy<int> lazy = []() {
                return 
                    Lazy<std::unique_ptr<int>>::computation([](){ return std::make_unique<int>(42); })
                    .map([](auto& ptr){ return *ptr; });
            }();


            REQUIRE( lazy.get() == 42 );
            REQUIRE( lazy.evaluated() == true );
        }
    }


    SECTION("Flat map") {
        auto m = [](char c){ 
            return Lazy<std::vector<char>>::construction(3, c); 
        };

        char chr = '*';
        std::vector<char> chrs{ '*', '*', '*' };

        SECTION("Correctly mapped") {
            auto mlazy = lazy.flat_map(m);

            REQUIRE( mlazy.get() == chrs );
        }

        SECTION("Evaluation orders") {
            auto mlazy = lazy.flat_map(m);

            REQUIRE( eval_count == 0 );
            REQUIRE( lazy.evaluated() == false );
            REQUIRE( mlazy.evaluated() == false );

            SECTION("Map evaluated before base") {
                REQUIRE( mlazy.get() == chrs );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );

                REQUIRE( lazy.get() == chr );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );
            }

            SECTION("Base evaluated before map") {
                REQUIRE( lazy.get() == chr );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == false );

                REQUIRE( mlazy.get() == chrs );

                REQUIRE( eval_count == 1 );
                REQUIRE( lazy.evaluated() == true );
                REQUIRE( mlazy.evaluated() == true );
            }
        }

        SECTION("Map can outlive base") {
            Lazy<std::vector<char>> lazy = [chr, m]() {
                return 
                    Lazy<std::unique_ptr<char>>::computation([chr](){ return std::make_unique<char>(chr); })
                    .map([](auto& ptr){ return *ptr; })
                    .flat_map(m);
            }();


            REQUIRE( lazy.get() == chrs );
            REQUIRE( lazy.evaluated() == true );
        }
    }
}

TEST_CASE("Lazy throws on recursive definition", "[lazy]") {
    Lazy<int> lazy = Lazy<int>::computation([&lazy](){ return lazy.get() + 1; });

    REQUIRE_THROWS_AS( lazy.get(), std::logic_error );
}