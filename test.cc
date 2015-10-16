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
  std::cout << test.get_domain() << std::endl;

  test_call((test.get_scheme() == "http"), "scheme");
  test_call((test.get_domain() == "www.example.com"), "domain");
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
    uri failing_user_pass(std::string("a:/bc@/"));
  }
  catch (std::invalid_argument iae)
  {
    std::cout << iae.what() << std::endl;
  }

  std::string stanza("a=");
  size_t key_value_divider = stanza.find_first_of('=');
  std::cout << stanza.substr(0, key_value_divider) << " : " << stanza.substr((key_value_divider + 1)) << std::endl;
  
  return 0;
}
