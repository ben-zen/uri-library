// Copyright (C) 2015 Ben Lewis <benjf5+github@gmail.com>
// Licensed under the MIT license.

#include "uri.hh"
#include <iostream>
#include <stdexcept>
#include <string>

void test_call(bool succeeded, char const *what)
{
  std::cout << (succeeded ? "PASSED: " : "FAILED: ") << what << std::endl;
}

int main()
{
  uri test(std::string("http://www.example.com/test?query#fragment"));
  std::cout << test.get_host() << std::endl;

  test_call((test.get_scheme() == "http"), "scheme");
  test_call((test.get_host() == "www.example.com"), "host");
  test_call((test.get_path() == "test"), "path");
  std::cout << test.get_path() << std::endl;
  test_call((test.get_query() == "query"), "query");
  std::cout << test.get_query() << std::endl;
  test_call((test.get_fragment() == "fragment"), "fragment");
  std::cout << test.get_fragment() << std::endl;


  uri no_path_test("http://www.example.com:8080/");
  test_call((no_path_test.get_path() == ""), "empty path");

  std::cout << test.to_string() << std::endl;

  std::cout << no_path_test.to_string() << std::endl;

  test.get_query_dictionary();

  uri no_host("file:/example.txt");
  std::cout << no_host.to_string() << std::endl;

  uri no_path_no_separator("https://www.example.com");
  std::cout << no_path_no_separator.to_string() << std::endl;

  // This next piece is from Wikipedia, as an example URN.
  std::cout << std::endl << "Checking some basic URN handling:" << std::endl;
  uri simple_urn("urn:example:mammal:monotreme:echidna");
  std::cout << simple_urn.to_string() << std::endl << std::endl;

  // Check for a broken scheme;
  try
  {
    uri failing_scheme(std::string("a"));
  }
  catch (std::invalid_argument iae)
  {
    std::cout << iae.what() << std::endl;
  }

  // Check for a broken username/password pair
  try
  {
    uri failing_user_pass(std::string("a://bc@/"));
  }
  catch (std::invalid_argument iae)
  {
    std::cout << iae.what() << std::endl;
  }
  
  return 0;
}
