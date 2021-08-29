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
    return tfl::is_nullable(tfl::derive(ls.cbegin(), ls.cend(), r));
}

TEST_CASE("Regex `accept`s benchmarking") {
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
    benchmark name                          samples     iterations    estimated
                                            mean        low mean      high mean
                                            std dev     low std dev   high std dev
    -------------------------------------------------------------------------------
    Using derivation and nullability               100             1     52.7016 s 
                                            331.601 ms    279.584 ms    378.946 ms 
                                            252.879 ms    235.151 ms    261.459 ms 
                                                                                
    Building the NFA                               100             1    566.739 ms 
                                            5.55533 ms    5.35502 ms    6.07012 ms 
                                            1.43431 ms    72.4793 us     2.6731 ms 
                                                                                
    Using a NFA                                    100             1     6.05765 s 
                                            50.578 ms    45.6174 ms    55.3715 ms 
                                            24.7726 ms    23.6128 ms    25.5217 ms 
                                                                                
    Building the DFA                               100             1     2.59341 s 
                                            25.4015 ms    25.1397 ms    25.9197 ms 
                                            1.81931 ms    1.12049 ms    3.18854 ms 
                                                                                
    Using a DFA                                    100             1    118.238 ms 
                                            1.17918 ms    1.17824 ms     1.1815 ms 
                                            7.11919 us    3.62309 us    14.5841 us 
 */