#include "uri.hh"
#include <iostream>
#include <string>

int main()
{
  uri test(std::string("http://www.example.com/test?query#fragment"));
  std::cout << test.domain << std::endl;
  return 0;
}
