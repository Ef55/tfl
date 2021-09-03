#include <iostream>
#include <fstream>
#include <cstdlib>

#include <tfl/Regex.hpp>
#include <tfl/AutomataOps.hpp>
#include <tfl/Graphs.hpp>

using Regex = tfl::Regex<char>;
using DFA = tfl::DFA<char>;
using NFA = tfl::NFA<char>;

int main(int argc, char** argv) {
    Regex regex = (Regex::literal('a')-Regex::literal('b')) | Regex::literal('c');

    DFA dfa = make_dfa(regex);
    NFA nfa = make_nfa(regex);

    std::ofstream dfile("dfa.dot");
    dfile << tfl::dot_graph(dfa) << std::endl;
    dfile.close();
    std::system("dot -Tpng dfa.dot -o dfa.png");

    std::ofstream nfile("nfa.dot");
    nfile << tfl::dot_graph(nfa) << std::endl;
    nfile.close();
    std::system("dot -Tpng nfa.dot -o nfa.png");

    return 0;
}