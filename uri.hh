// Copyright (C) 2015 Ben Lewis <benjf5+github@gmail.com>
// Licensed under the MIT license.

#pragma once
#include <map>
#include <string>
#include <stdexcept>

class uri
{
  /* URIs are broadly divided into two categories: hierarchical and
   * non-hierarchical. Both hierarchical URIs and non-hierarchical URIs have a
   * few elements in common; all URIs have a scheme of one or more alphanumeric
   * characters followed by a colon, and they all may optionally have a query
   * component preceded by a question mark, and a fragment component preceded by
   * an octothorpe (hash mark: '#'). The query consists of stanzas separated by
   * ampersands ('&'), and each stanza consists of a key and an optional value;
   * if the value exists, the key and value must be divided by an equals
   * sign. CGI applications are encouraged to also support semicolons as
   * dividers in their query strings, this library currently does not support
   * the use of semicolons.
   *
   * The following is an example from Wikipedia of a hierarchical URI:
   * scheme:[//[user:password@]domain[:port]][/]path[?query][#fragment]
   */

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

  std::string const &get_content() const
  {
    if (m_category != scheme_category::NonHierarchical)
    {
      throw std::domain_error("The content component is only valid for non-hierarchical URIs.");
    }
    return m_content;
  };

  std::string const &get_username() const
  {
    if (m_category != scheme_category::Hierarchical)
    {
      throw std::domain_error("The username component is only valid for hierarchical URIs.");
    }
    return m_username;
  };

  std::string const &get_password() const
  {
    if (m_category != scheme_category::Hierarchical)
    {
      throw std::domain_error("The password component is only valid for hierarchical URIs.");
    }
    return m_password;
  };
  
  std::string const &get_host() const
  {
    if (m_category != scheme_category::Hierarchical)
    {
      throw std::domain_error("The host component is only valid for hierarchical URIs.");
    }
    return m_host;
  };

  unsigned long get_port() const
  {
    if (m_category != scheme_category::Hierarchical)
    {
      throw std::domain_error("The port component is only valid for hierarchical URIs.");
    }
    return m_port;
  };

  std::string const &get_path() const
  {
    if (m_category != scheme_category::Hierarchical)
    {
      throw std::domain_error("The path component is only valid for hierarchical URIs.");
    }
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

    if (m_content.length() > m_path.length())
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
  };
  
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

    size_t content_end = std::min(query_token_location, 
                                  fragment_token_location);
    m_content = uri_text.substr((scheme_end + 1),
                                ((content_end != std::string::npos)
                                 ? (content_end - 1 - scheme_end)
                                 : std::string::npos));

    // The parsing of the authority component differs between hierarchical and non-hierarchical URIs.
    switch (category)
    {
    case scheme_category::Hierarchical:
      if (m_content.length() > 0)
      {
	size_t path_start = std::string::npos;
	if (!m_content.compare(0, 2, "//"))
	{
	  // In this case, we have a host and possibly additional data; parse it in chunks.
	  size_t host_start = 2;
	  size_t user_pass_end = m_content.find_first_of('@');
	  if (user_pass_end != std::string::npos)
	  {
	    size_t user_pass_divider = m_content.find_first_of(':');
	    if (user_pass_divider > user_pass_end)
	    {
	      throw std::invalid_argument("Could not parse the username/password section of the supplied URI. Supplied URI was missing a partition between username and password components.");
	    }

	    m_username = m_content.substr(0, user_pass_divider);
	    m_password = m_content.substr((user_pass_divider + 1), (user_pass_end - user_pass_divider - 1));
	    host_start = user_pass_end + 1;
	  }

	  path_start = m_content.find_first_of('/', host_start);
	  size_t port_indicator = m_content.find_first_of(':', host_start);
	  if (port_indicator != std::string::npos)
	  {
	    m_port = std::stoul(m_content.substr((port_indicator + 1),
                                                 ((path_start != std::string::npos)
                                                  ? (path_start - port_indicator - 1)
                                                  : std::string::npos)));
	  }

	  size_t host_end = std::min(path_start, port_indicator);
	  m_host = m_content.substr(host_start, (host_end - host_start));
	  if (path_start != std::string::npos)
	  {
	    path_start++;
	  }
	  m_path_is_rooted = true;
	}
	else
	{
	  if (!(m_content.compare(0, 1, "/")))
	  {
	    path_start = 1;
	    m_path_is_rooted = true;
	  }
	  else
	  {
	    path_start = 0;
	  }
	}
	m_path = (path_start < m_content.length()) ? m_content.substr(path_start) : "";
      }
      break;
    case scheme_category::NonHierarchical:
      // Included for completeness; the content component of non-hierarchical
      // URIs is not parsed by this class; in the future specializations of this
      // class might support specific cases, but that will be on a case-by-case
      // basis.
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
  std::string m_content;
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
