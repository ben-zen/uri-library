# URIs for Modern C++ #
A header-only URI library in modern C++. Just plug and play!

## URI support ##
The driving reason for this library is the general lack of URI-parsing libraries
that are generic to all forms of URIs, and the amount of bad URI parsing in the
world. To address both of those cases, this library provides access to the
content component (called `hier-part` in the spec, even for non-hierarchical
URIs) for non-hierarchical URIs, and provides access to the parsed components of
the content component (username, password, host, port, path) for hierarchical
URIs. Note that accessing hierarchical URI components for a non-hierarchical URI
is invalid and will throw a `std::domain_error`, while accessing the content
component of a hierarchical URI is invalid and will likewise throw a
`std::domain_error`. For additional notes, see the following API documentation.

## URI API ##

### Constructors ###
* `uri(char const *uri_text, scheme_category category =
  scheme_category::Hierarchical)` and `uri(std::string const &uri_text,
  scheme_category category = scheme_category::Hierarchical)`: constructs
  a `uri` object and throws an exception for any invalid component.

### Accessors ###
* `std::string const &get_scheme() const`: get the scheme component.
* `scheme_category get_scheme_category() const`: get the scheme category, either
  Hierarchical or NonHierarchical.
* `std::string const &get_content() const`: get the content component of a
  non-hierarchical URI. Throws when called on a hierarchical URI.
* `std::string const &get_username() const`: get the username component of the
  URI (or return an empty string if none was present in the source string.) This
  method will most likely be marked deprecated shortly (as username/password
  handling in URIs is deprecated.) Throws when called on a non-hierarchical
  URI.
* `std::string const &get_password() const`: get the password component of the
  URI (or return an empty string if none was present in the source string.) This
  method will most likely be marked deprecated shortly (as username/password
  handling in URIs is deprecated.) Throws when called on a non-hierarchical
  URI.
* `std::string const &get_host() const`: get the host component of the
  URI. Returns an empty string if the host component was empty or not
  supplied. Throws when called on a non-hierarchical URI.
* `unsigned long get_port() const`: get the port component (parsed into an
  `unsigned long`) of the URI. If no port was supplied, returns 0. Throws when
  called on a non-hierarchical URI.
* `std::string const &get_path() const`: get the path component of the
  URI. Returns an empty string if the path component was empty. Throws when
  called on a non-hierarchical URI.
* `std::string const &get_query() const`: get the query component of the URI, as
  a string. Returns an empty string if no query was supplied.
* `std::map<std::string, std::string> const &get_query_dictionary() const`: get
  the parsed contents of the query component, as a key-value
  dictionary. Currently this library only supports query strings with ampersands
  (`&`) as dividers, future versions may support semicolons. This dictionary
  will most likely be invalid for query strings using semicolons and should be
  ignored in that scenario.
* `std::string const &get_fragment() const`: get the fragment component of the
  URI. Returns an empty string if no fragment was supplied.
* `std::string to_string() const`: get the normalized form of the URI; any
  empty components included in the initial URI string will be stripped from this
  form. Currently does not normalize on capitalization, but do not rely on the
  case of the returned URI matching the supplied case in the future. If no
  authority component was present in a hierarchical URI, this method will
  preserve the rootless state of the path component, i.e. for the URN
  `urn:ietf:rfc:2141`, the path is `ietf:rfc:2141` with no root (initial `/`
  character) and as such, the normalized string form also will not have a root
  character.

## Building tests ##
This library comes with a basic set of tests, which (as of this writing) mostly
confirm that a few example URIs work well, and should confirm the operation of
the library for various cases. Instructions for building and running the tests
with GCC:

    g++ -std=c++11 test.cc -o uri_test
    ./uri_test

(You can substitute `clang++` when your clang installation includes C++11
support. I haven't tested with MSVC yet.)

