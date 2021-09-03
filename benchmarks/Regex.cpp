#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "tfl/Regex.hpp"
#include "tfl/AutomataOps.hpp"

using Regex = tfl::Regex<char>;
using Regexes = tfl::Regexes<char>;
using NFA = tfl::NFA<char>;
using DFA = tfl::DFA<char>;

static bool daccept(Regex const& r, std::vector<char> ls) {
    return tfl::is_nullable(tfl::derive(ls, r));
}

TEST_CASE("Regex `accept`s benchmarking", "[regex]") {
    using Catch::Generators::chunk;
    using Catch::Generators::random;

    auto generator = chunk(10000, random('a', 'd'));

    // Generate a regex
    Regex a = Regex::literal('a');
    Regex b = Regex::literal('b');
    Regex c = Regex::literal('c');
    Regex d = Regex::literal('d');
    Regex eps = Regex::epsilon();
    Regex alph = Regex::alphabet();
    Regex any = Regex::any();

    Regex r1 = *(a|b|c) / *(a-Regexes::opt(b|c));
    Regex r2 = *((a|b) - (c|d) - Regexes::opt(alph)) / (any-d-a-any);

    Regex regex = r1-any-r2;
    NFA nfa = tfl::make_nfa<char>(regex);
    DFA dfa = tfl::make_dfa<char>(regex);

    static constexpr int COUNT = 100;
    static constexpr int LOW = 10;
    int i = 0;

    for(int j = 0; j < COUNT; ++j) {
        auto input = generator.get();

        bool b1(daccept(regex, input));
        bool b2(nfa.accepts(input));
        bool b3(dfa.accepts(input));

        INFO("Verify that the different regex matchers yield the same results");
        INFO("Derivation: " << b1);
        INFO("NFA: " << b2);
        INFO("DFA: " << b3);
        REQUIRE(b1 == b2);
        REQUIRE(b2 == b3);

        if(b1) {
            ++i;
        }
        generator.next();
    }

    INFO("Verify that the generated strings are sometimes accepted, sometimes rejected.");
    std::cout << "Test run: " << i << " accepted, " << (COUNT - i) << " rejected." << std::endl;
    REQUIRE( (LOW <= i && i <= (COUNT - LOW)) );
    

    BENCHMARK_ADVANCED("Using derivation and nullability")(Catch::Benchmark::Chronometer meter) {
        std::vector<std::vector<char>> data(meter.runs());
        std::generate(data.begin(), data.end(), [&generator](){ auto v = generator.get(); generator.next(); return v; });
        meter.measure([&regex, &data](int i) { return daccept(regex, data[i]); });
    };

    BENCHMARK("Building the NFA") {
        return tfl::make_nfa<char>(regex);
    };

    BENCHMARK_ADVANCED("Using a NFA")(Catch::Benchmark::Chronometer meter) {
        std::vector<std::vector<char>> data(meter.runs());
        std::generate(data.begin(), data.end(), [&generator](){ auto v = generator.get(); generator.next(); return v; });
        meter.measure([&nfa, &data](int i) { return nfa.accepts(data[i]); });
    };

    BENCHMARK("Building the DFA") {
        return tfl::make_dfa<char>(regex);
    };

    BENCHMARK_ADVANCED("Using a DFA")(Catch::Benchmark::Chronometer meter) {
        std::vector<std::vector<char>> data(meter.runs());
        std::generate(data.begin(), data.end(), [&generator](){ auto v = generator.get(); generator.next(); return v; });
        meter.measure([&dfa, &data](int i) { return dfa.accepts(data[i]); });
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
    Using derivation and nullability               100             1     20.1902 s 
                                            252.992 ms    212.709 ms    289.626 ms 
                                            196.867 ms    183.918 ms    202.775 ms 
                                                                                
    Building the NFA                               100             1    274.082 ms 
                                            2.73796 ms    2.73441 ms    2.74352 ms 
                                            22.2374 us    16.2341 us    32.5903 us 
                                                                                
    Using a NFA                                    100             1     2.90752 s 
                                            24.0109 ms     21.206 ms     26.803 ms 
                                            13.8937 ms     13.286 ms    14.0344 ms 
                                                                                
    Building the DFA                               100             1     1.32351 s 
                                            13.3133 ms    13.2843 ms    13.3509 ms 
                                            167.676 us    135.971 us    210.338 us 
                                                                                
    Using a DFA                                    100             1    102.352 ms 
                                            1.02379 ms    1.02276 ms    1.02507 ms 
                                            5.81902 us    4.77966 us    7.25158 us
 */