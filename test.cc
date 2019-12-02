// Copyright (C) 2015 Ben Lewis <benjf5+github@gmail.com>
// Licensed under the MIT license.

#include "uri.hh"
#include <iostream>
#include <stdexcept>
#include <string>

void test_call(bool succeeded, char const *what)
{
  std::cout << (succeeded ? "PASSED: " : "FAILED: ") << what << std::endl << std::endl;
}

void test_scheme()
{
  std::string bad_scheme("http");
  std::string no_length_scheme(":abc");
  std::string only_scheme("http:");

  std::cout << "Testing the scheme parsing component." << std::endl << std::endl;

  try
  {
    uri test_uri(bad_scheme);
  }
  catch (std::invalid_argument iae)
  {
    std::cout << "Caught expected failure with a malformed scheme section (missing end-colon):" 
	      << std::endl
	      << iae.what()
	      << std::endl
	      << std::endl;
  }

  try
  {
    uri test_uri(no_length_scheme);
  }
  catch (std::invalid_argument iae)
  {
    std::cout << "Caught expected failure with a malformed scheme section (zero-length scheme):" 
	      << std::endl
	      << iae.what()
	      << std::endl
	      << std::endl;
  }

  try
  {
    uri test_uri(only_scheme);
    std::cout << "Constructed expected URI with only a scheme." << std::endl
	      << "URI is: " << test_uri.to_string() << std::endl << std::endl;

    test_call((test_uri.get_scheme() == "http"), "Captured expected scheme: \"http\".");
  }
  catch (std::invalid_argument iae)
  {
    std::cout << "Caught unexpected exception in constructing the URI: " << only_scheme
	      << std::endl
	      << "Exception states: " << iae.what()
	      << std::endl
	      << std::endl;
  }
}

int main()
{
  std::cout << "Running the URI library test suite ..." << std::endl << std::endl;
  
  test_scheme();

  uri test(std::string("http://www.example.com/test?query#fragment"));
  std::cout << test.get_host() << std::endl;

  test_call((test.get_host() == "www.example.com"), "host");
  test_call((test.get_path() == "test"), "path");
  std::cout << test.get_path() << std::endl;
  test_call((test.get_query() == "query"), "query");
  std::cout << test.get_query() << std::endl;
  test_call((test.get_fragment() == "fragment"), "fragment");
  std::cout << test.get_fragment() << std::endl << std::endl;

  std::cout << "Testing IPv6 support." << std::endl;
  uri ipv6_test("http://[::1]:8080/");
  std::cout << ipv6_test.get_host() << std::endl;
  std::cout << ipv6_test.get_port() << std::endl << std::endl;

  uri empty_path_test("http://www.example.com:8080/");
  test_call((empty_path_test.get_path() == ""), "empty path");

  uri no_path_test("http://www.example.com:8080");
  test_call((no_path_test.get_path() == ""), "no path");

  std::cout << test.to_string() << std::endl;

  std::cout << empty_path_test.to_string() << std::endl;
  std::cout << no_path_test.to_string() << std::endl;

  test.get_query_dictionary();

  uri no_host("file:/example.txt");
  std::cout << no_host.to_string() << std::endl;

  uri no_path_no_separator("https://www.example.com");
  std::cout << no_path_no_separator.to_string() << std::endl;

  std::cout << std::endl << "Checking some basic URN handling:" << std::endl;
  uri simple_urn("urn:ietf:rtc:2141", uri::scheme_category::Hierarchical);
  std::cout << simple_urn.to_string() << std::endl << std::endl;
  std::cout << simple_urn.get_path() << std::endl << std::endl;

  // Testing the copy constructor.
  uri copied_urn(simple_urn);
  std::cout << copied_urn.to_string() << std::endl << std::endl;

  // Check for a broken username/password pair
  try
  {
    uri failing_user_pass(std::string("a://bc@/"));
  }
  catch (std::invalid_argument iae)
  {
    std::cout << iae.what() << std::endl << std::endl;
  }

  // Testing an abnormal URI that other parsers fail on
  uri abnormal_path_uri("http://a/b/c/g;x=1/y");
  std::cout << "Checking path parsing for an abnormal path:"
            << std::endl
            << abnormal_path_uri.get_path()
            << std::endl << std::endl;

  uri query_test("http://a/b/?c=1&d=2");
  std::cout << "Checking the query map for a basic test URI." << std::endl;
  auto query_dict = query_test.get_query_dictionary();
  test_call((query_dict["c"] == "1"), "Testing for a value in the query dictionary.");
  test_call((query_dict["d"] == "2"), "Testing for a value in the query dictionary.");
  
  uri query_test_semicolon("http://a/b/?c=1;d=2", uri::scheme_category::Hierarchical, 
                           uri::query_argument_separator::semicolon);
  std::cout << "Checking the query map for a basic test URI that uses semicolons." << std::endl;
  test_call((query_dict["c"] == "1"), "Testing for a value in the query dictionary.");
  test_call((query_dict["d"] == "2"), "Testing for a value in the query dictionary.");

  // Testing a non-hierarchical URI:
  uri data_uri("data:text/html,<!DOCTYPE html><html><head><title>test</title></head><body><h1>testing</h1><p>Test.</p></body></html>",
               uri::scheme_category::NonHierarchical);
  std::cout << "Checking out handling of a non-hierarchical URI:" << std::endl
            << data_uri.get_content()
            << std::endl << std::endl;
  
  return 0;
}
