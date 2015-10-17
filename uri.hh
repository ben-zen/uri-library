// Copyright (C) 2015 Ben Lewis <benjf5+github@gmail.com>
// Licensed under the MIT license.

#pragma once
#include <map>
#include <string>
#include <stdexcept>

class uri
{

  // scheme:[//[user:password@]domain[:port]][/]path[?query][#fragment] (from Wikipedia)

public:

  enum class scheme_category
  {
    Hierarchical,
    NonHierarchical
  };

  uri(char const *uri_text, scheme_category category = scheme_category::Hierarchical) :
    m_category(category),
    m_path_is_rooted(false),
    m_port(0)
  {
    setup(std::string(uri_text), category);
  };

  uri(std::string const &uri_text, scheme_category category = scheme_category::Hierarchical) : 
    m_category(category),
    m_path_is_rooted(false),
    m_port(0)
  {
    setup(uri_text, category);
  };

  ~uri() { };

  std::string const &get_scheme() const
  {
    return m_scheme;
  };

  scheme_category get_scheme_category() const
  {
    return m_category;
  };

  std::string const &get_username() const
  {
    return m_username;
  };

  std::string const &get_password() const
  {
    return m_password;
  };
  
  std::string const &get_host() const
  {
    return m_host;
  };

  unsigned long get_port() const
  {
    return m_port;
  };

  std::string const &get_path() const
  {
    return m_path;
  };

  std::string const &get_query() const
  {
    return m_query;
  };

  std::map<std::string, std::string> const &get_query_dictionary() const
  {
    return m_query_dict;
  };

  std::string const &get_fragment() const
  {
    return m_fragment;
  };

  std::string to_string() const
  {
    std::string full_uri;
    full_uri.append(m_scheme);
    full_uri.append(":");

    if (m_authority.length() > m_path.length())
    {
      full_uri.append("//");
      if (!(m_username.empty() || m_password.empty()))
      {
        full_uri.append(m_username);
        full_uri.append(":");
        full_uri.append(m_password);
        full_uri.append("@");
      }

      full_uri.append(m_host);

      if (m_port != 0)
      {
        full_uri.append(":");
        full_uri.append(std::to_string(m_port));
      }
    }

    if (m_path_is_rooted)
    {
      full_uri.append("/");
    }
    full_uri.append(m_path);

    if (!m_query.empty())
    {
      full_uri.append("?");
      full_uri.append(m_query);
    }

    if (!m_fragment.empty())
    {
      full_uri.append("#");
      full_uri.append(m_fragment);
    }

    return full_uri;
  }
  
private:

  void setup(std::string const &uri_text, scheme_category category)
  {
    size_t carat = 0;
    size_t const uri_length = uri_text.length();

    if (uri_length == 0)
    {
      throw std::invalid_argument("URIs cannot be of zero length.");
    }

    size_t scheme_end = uri_text.find_first_of(':');
    if ((scheme_end == std::string::npos) || (scheme_end == 0))
    {
      throw std::invalid_argument("Could not find the scheme of the supplied URI. Supplied URI was: " + uri_text);
    }

    m_scheme = uri_text.substr(carat, (scheme_end - carat));
    carat = scheme_end; // scheme_end is currently at the first ':'

    size_t query_token_location = uri_text.find_first_of('?');
    size_t fragment_token_location = uri_text.find_first_of('#');
    size_t authority_section_end = std::min(query_token_location, fragment_token_location);

    if (fragment_token_location != std::string::npos)
    {
      // It's the last component of the URI, so I'm willing to be lazy
      // and use std::string::npos for the length.
      m_fragment = uri_text.substr(fragment_token_location + 1, std::string::npos);
    }

    if (query_token_location != std::string::npos)
    {
      if ((fragment_token_location != std::string::npos) && (fragment_token_location < query_token_location))
      {
	throw std::invalid_argument("The order of the fragment and the query is inverted in the supplied URI.");
      }

      m_query = uri_text.substr((query_token_location + 1),
				((fragment_token_location != std::string::npos)
				 ? (fragment_token_location - 1 - query_token_location)
				 : std::string::npos));
    }

    init_query_dictionary(); // If the query string is empty, this will be empty too.

    m_authority = uri_text.substr((scheme_end + 1),
				  ((authority_section_end != std::string::npos)
				   ? (authority_section_end - 1 - scheme_end)
				   : std::string::npos));

    // The parsing of the authority component differs between hierarchical and non-hierarchical URIs.
    switch (category)
    {
    case scheme_category::Hierarchical:
      if (m_authority.length() > 0)
      {
	size_t path_start = std::string::npos;
	if (!m_authority.compare(0, 2, "//"))
	{
	  // In this case, we have a host and possibly additional data; parse it in chunks.
	  size_t host_start = 2;
	  size_t user_pass_end = m_authority.find_first_of('@');
	  if (user_pass_end != std::string::npos)
	  {
	    size_t user_pass_divider = m_authority.find_first_of(':');
	    if (user_pass_divider > user_pass_end)
	    {
	      throw std::invalid_argument("Could not parse the username/password section of the supplied URI. Supplied URI was missing a partition between username and password components.");
	    }

	    m_username = m_authority.substr(0, user_pass_divider);
	    m_password = m_authority.substr((user_pass_divider + 1), (user_pass_end - user_pass_divider - 1));
	    host_start = user_pass_end + 1;
	  }

	  path_start = m_authority.find_first_of('/', host_start);
	  size_t port_indicator = m_authority.find_first_of(':', host_start);
	  if (port_indicator != std::string::npos)
	  {
	    m_port = std::stoul(m_authority.substr((port_indicator + 1),
						   ((path_start != std::string::npos)
						    ? (path_start - port_indicator - 1)
						    : std::string::npos)));
	  }

	  size_t host_end = std::min(path_start, port_indicator);
	  m_host = m_authority.substr(host_start, (host_end - host_start));
	  if (path_start != std::string::npos)
	  {
	    path_start++;
	  }
	  m_path_is_rooted = true;
	}
	else
	{
	  if (!(m_authority.compare(0, 1, "/")))
	  {
	    path_start = 1;
	    m_path_is_rooted = true;
	  }
	  else
	  {
	    path_start = 0;
	  }
	}
	m_path = (path_start < m_authority.length()) ? m_authority.substr(path_start) : "";
      }
      break;
    case scheme_category::NonHierarchical:
      throw std::invalid_argument("Non-hierarchical URIs are currently not supported by this library. The supplied URI was: " + uri_text);
      break;
    }
  };

  void init_query_dictionary()
  {
    if (!m_query.empty())
    {
      // Loop over the query string looking for '&'s, then check each one for
      // an '=' to find keys and values; if there's not an '=' then the key
      // will have an empty value in the map.
      size_t carat = 0;
      size_t stanza_end = m_query.find_first_of('&');
      do
      {
	std::string stanza = m_query.substr(carat, ((stanza_end != std::string::npos) ? (stanza_end - carat) : std::string::npos));
	size_t key_value_divider = stanza.find_first_of('=');
	std::string key = stanza.substr(0, key_value_divider);
	std::string value;
	if (key_value_divider != std::string::npos)
	{
	  value = stanza.substr((key_value_divider + 1));
	}

	if (m_query_dict.count(key) != 0)
	{
	  throw std::invalid_argument("Bad key in the query string!");
	}

	m_query_dict.emplace(key, value);
	carat = ((stanza_end != std::string::npos) ? (stanza_end + 1)
		 : std::string::npos);
	stanza_end = m_query.find_first_of('&', carat);
      }
      while ((stanza_end != std::string::npos) 
	     || (carat != std::string::npos));
    }
  }

  std::string m_scheme;
  std::string m_authority;
  std::string m_username;
  std::string m_password;
  std::string m_host;
  std::string m_path;
  std::string m_query;
  std::string m_fragment;

  std::map<std::string, std::string> m_query_dict;

  scheme_category m_category;
  unsigned long m_port;
  bool m_path_is_rooted;
};
