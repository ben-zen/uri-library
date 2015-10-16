# URIs for Modern C++ #
A header-only URI library in modern C++. Just plug and play!

## Building tests ##
This library comes with a basic set of tests, which (as of this writing) mostly
confirm that a few example URIs work well, and should confirm the operation of
the library for various cases.

To build and run the tests,
    g++ -std=c++11 test.cc -o uri_test
    ./uri_test

works well on platforms with GCC.
