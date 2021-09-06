# TFL - A Terrible Front-end Library

TFL is a a lexing and parsing library which was meant to be as concise and simple as possible (thus terribly inefficient, hence the name), but outgrew its scope.

## Features

- Lexers using extended regexes for tokens specification:
    - Derivative-based lexer, which is simple, but terribly slow;
    - DFA-based lexer, which is fast, but needs some time to be built;
- Parser with a parser-combinator-like interface:
    -  "Brute force" recursive descent parser, which is terrible but accepts almost all Context-free grammars;
- Extras:
    - DFA/NFA creation interface;
    - Regex/DFA/NFA graph generation.

## Documentation

The documentation can be generated using
`cmake --target Doc`
(requires [doxygen](https://www.doxygen.nl/index.html)).

## Examples

This repository contains some examples showing how to use TFL:
- *Calculator*: a simple 1 file, < 150 lines example;
- *Graphs*: shows how dot graphs can be outputed and then made into pictures;
- *Json*: a "real world" use case;
- *Kaleidoscope*: is a more complex example taken from the [llvm tutorial](https://llvm.org/docs/tutorial/).