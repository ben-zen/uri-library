# URIs for Modern C++ #
A header-only URI library in modern C++. Just plug and play!

## Building tests ##
This library comes with a basic set of tests, which (as of this writing) mostly
confirm that a few example URIs work well, and should confirm the operation of
the library for various cases. Instructions for building and running the tests
with GCC:

    g++ -std=c++11 test.cc -o uri_test
    ./uri_test

(You can substitute `clang++` when your clang installation includes C++11
support. I haven't tested with MSVC yet.)

