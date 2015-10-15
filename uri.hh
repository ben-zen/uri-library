// Copyright (C) 2015 Ben Lewis <benjf5+github@gmail.com>

#pragma once
#include <string>
#include <stdexcept>

class uri
{

  // scheme:[//[user:password@]domain[:port]][/]path[?query][#fragment] (from Wikipedia)

public:
  uri(std::string&& uri_text)
  {
    size_t carat = 0;
    size_t const uri_length = uri_text.length();

    if (uri_length == 0)
    {
      throw std::invalid_argument("URIs cannot be of zero length.");
    }
    
    size_t scheme_end = uri_text.find_first_of(':');
    if ((scheme_end == std::string::npos) || (scheme_end == uri_length))
    {
      throw std::invalid_argument("Could not find the scheme of the supplied URI. Supplied Uri was: " + uri_text);
    }

    scheme = uri_text.substr(carat, (scheme_end - carat));
    carat = scheme_end; // scheme_end is currently ':'
    if ((carat + 2) < uri_length)
    {
      if ((uri_text[++carat] == '/') // If yes, then check the next one, if no, we're already in the path.
	  && (uri_text[++carat] == '/')) // Again, if yes, then we're in the username/password/domain block, if no, then path.
      {
	carat++;
	size_t user_pass_end = uri_text.find_first_of('@', carat);
	if (user_pass_end != std::string::npos) // Now we parse the username/password block
	{
	  size_t user_pass_divider = uri_text.find_first_of(':', carat);
	  if ((user_pass_divider == std::string::npos) || (user_pass_divider > user_pass_end))
	  {
	    throw std::invalid_argument("Could not parse the username/password section of the supplied URI. Supplied URI was: " + uri_text);
	  }

	  username = uri_text.substr(carat, (user_pass_divider - carat));
	  carat = user_pass_divider + 1;
	  password = uri_text.substr(carat, (user_pass_end - carat));
	  carat = user_pass_end + 1;
	}

	size_t authority_path_divider = uri_text.find_first_of('/', carat);       
	if (authority_path_divider == std::string::npos)
	{
	  throw std::invalid_argument("URI has domain section but no path. Supplied URI was: " + uri_text);
	}

	size_t end_of_domain = authority_path_divider;

	// Here we can parse the domain, such as it requires parsing. Check first for the presence of a port:
	size_t port_indicator = uri_text.find_first_of(':', carat);

	if (port_indicator != std::string::npos)
	{
	  size_t local_carat = port_indicator + 1;
	  std::string port_text = uri_text.substr(local_carat, (authority_path_divider - local_carat));
	  port = std::stoul(port_text);
	  end_of_domain = port_indicator;
	}

	domain = uri_text.substr(carat, (end_of_domain - carat));
	carat = authority_path_divider + 1; // Now it sits at the start of the path.
      }

      size_t start_of_fragment = uri_text.find_first_of('#', carat);
      if (start_of_fragment != std::string::npos)
      {
	// In this case the fragment is present, and we might as well extract it now.
	fragment = uri_text.substr(start_of_fragment + 1);
      }

      size_t start_of_query = uri_text.find_first_of('?', carat);
      if (start_of_query != std::string::npos)
      {
	// Now we have the start of the query, and we also know if there's a fragment, and where it starts.
	query = uri_text.substr((start_of_query + 1), ((start_of_fragment != std::string::npos) ? (start_of_fragment - 1) : std::string::npos));
      }

      // Now that we have the fragment and query out of the way, what remains is the path.
      path = uri_text.substr(carat, ((start_of_query != std::string::npos) ? (start_of_query - 1) : std::string::npos));
    }
	
  };
  ~uri() { };

  // private:
  std::string scheme;
  std::string authority_component;
  std::string username;
  std::string password;
  std::string domain;
  std::string path;
  std::string query;
  std::string fragment;

  unsigned long port;
};
