#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "tfl/Automata.hpp"

using DFA = tfl::DFA<char>;
static constexpr auto DEAD_STATE = DFA::DEAD_STATE;

TEST_CASE("DEAD state allows early termination", "[DFA]") {
    using Catch::Generators::chunk;
    using Catch::Generators::random;
    using Catch::Generators::map;

    std::vector<char> char_prob(1000000, 'b');
    char_prob[char_prob.size()-1] = 'a';


    auto generator = chunk(100000, 
        map(
            [char_prob](auto i){ return char_prob[i]; },
            random(0, (int)char_prob.size()-1)
        )
    );

    DFA defa = DFA::Builder({'a'}, 3)
        .set_transition(0, 'a', 1)
        .set_unknown_transition(0, 2)
        .set_transition(1, 'a', 1)
        .set_unknown_transition(1, 2)
        .set_all_transitions(2, 2)
        .set_acceptance(1, true);

    DFA dead = DFA::Builder({'a'}, 2)
        .set_transition(0, 'a', 1)
        .set_unknown_transition(0, DEAD_STATE)
        .set_transition(1, 'a', 1)
        .set_unknown_transition(1, DEAD_STATE)
        .set_acceptance(1, true);


    static constexpr int COUNT = 10;
    for(int j = 0; j < COUNT; ++j) {
        auto input = generator.get();

        bool b1(defa.accepts(input));
        bool b2(dead.accepts(input));
        auto r1(defa.munch(input));
        auto r2(dead.munch(input));

        INFO("Verify that the different regex matchers yield the same results");
        INFO("DEFAULT: " << Catch::StringMaker<decltype(r1)>::convert(r1));
        INFO("DEAD: " << Catch::StringMaker<decltype(r2)>::convert(r2));
        REQUIRE(b1 == b2);
        REQUIRE(r1 == r2);

        generator.next();
    }

    std::vector<std::vector<char>> data(1000);
    std::generate(data.begin(), data.end(), [&generator](){ auto v = generator.get(); generator.next(); return v; });

    BENCHMARK_ADVANCED("Using a normal state as dead state (Accept)")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&defa, &data](int i) { return defa.accepts(data[i % data.size()]); });
    };

    BENCHMARK_ADVANCED("Using the special DEAD state (Accept)")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&dead, &data](int i) { return dead.accepts(data[i % data.size()]); });
    };

    BENCHMARK_ADVANCED("Using a normal state as dead state (Munch)")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&defa, &data](int i) { return defa.munch(data[i % data.size()]); });
    };

    BENCHMARK_ADVANCED("Using the special DEAD state (Munch)")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&dead, &data](int i) { return dead.munch(data[i % data.size()]); });
    };
}


/*
Specs:
    OS: Debian GNU/Linux 10 (buster) x86_64 
    Host: MS-7977 1.0 
    Kernel: 4.19.0-17-amd64 
    CPU: Intel i5-6600K (4) @ 3.900GHz 
    Memory: 15997MiB
    Compiler: GCC 11.2.0

Result:
    ...............................................................................
    benchmark name                          samples      iterations    estimated
                                            mean         low mean      high mean
                                            std dev      low std dev   high std dev
    -------------------------------------------------------------------------------
    Using a normal state as dead state                                             
    (Accept)                                       100             1    659.513 ms 
                                            6.60142 ms    6.59528 ms    6.60776 ms 
                                              31.69 us    28.0786 us    36.5329 us 
                                                                                
    Using the special DEAD state                                                   
    (Accept)                                       100           533     5.7031 ms 
                                             107.41 ns    106.745 ns    109.871 ns 
                                             5.8192 ns    1.42948 ns    13.5065 ns 
                                                                                
    Using a normal state as dead state                                             
    (Munch)                                        100             1     1.07138 s 
                                             10.711 ms    10.7071 ms    10.7155 ms 
                                             21.378 us    18.6512 us    24.7861 us 
                                                                                
    Using the special DEAD state                                                   
    (Munch)                                        100           529     5.7132 ms 
                                            108.933 ns    108.271 ns      111.3 ns 
                                            5.74008 ns    1.58462 ns    13.1893 ns                                                  
 */