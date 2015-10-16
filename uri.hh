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

  uri(char const *uri_text) :
    m_port(0), 
    m_query_dict_initialized(false)
  {
    setup(std::string(uri_text));
  }

  uri(std::string const &uri_text) : 
    m_port(0), 
    m_query_dict_initialized(false)
  {
    setup(uri_text);
  }

  ~uri() { };

  std::string const &get_scheme() const
  {
    return m_scheme;
  }

  std::string const &get_username() const
  {
    return m_username;
  }

  std::string const &get_password() const
  {
    return m_password;
  }
  
  std::string const &get_domain() const
  {
    return m_domain;
  }

  unsigned long get_port() const
  {
    return m_port;
  }
  
  std::string const &get_path() const
  {
    return m_path;
  }

  std::string const &get_query() const
  {
    return m_query;
  }

  std::map<std::string, std::string> const &get_query_dictionary()
  {
    if (!m_query_dict_initialized)
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
    m_query_dict_initialized = true;
    return m_query_dict;
  }

  std::string const &get_fragment() const
  {
    return m_fragment;
  }

  std::string to_string() const
  {
    std::string full_uri;
    full_uri.append(m_scheme);
    full_uri.append(":");
    if (!m_domain.empty())
    {
      full_uri.append("//");
      if (!(m_username.empty() || m_password.empty()))
      {
        full_uri.append(m_username);
        full_uri.append(":");
        full_uri.append(m_password);
        full_uri.append("@");
      }

      full_uri.append(m_domain);

      if (m_port != 0)
      {
        full_uri.append(":");
        full_uri.append(std::to_string(m_port));
      }
    }
    full_uri.append("/");
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

  void setup(std::string const &uri_text)
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

    m_scheme = uri_text.substr(carat, (scheme_end - carat));
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

	  m_username = uri_text.substr(carat, (user_pass_divider - carat));
	  carat = user_pass_divider + 1;
	  m_password = uri_text.substr(carat, (user_pass_end - carat));
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
	  m_port = std::stoul(port_text);
	  end_of_domain = port_indicator;
	}

	m_domain = uri_text.substr(carat, (end_of_domain - carat));
	carat = authority_path_divider + 1; // Now it sits at the start of the path.
      }

      size_t end_of_query = std::string::npos;
      size_t start_of_fragment = uri_text.find_first_of('#', carat);
      if (start_of_fragment != std::string::npos)
      {
	// In this case the fragment is present, and we might as well extract it now.
	m_fragment = uri_text.substr(start_of_fragment + 1);
        end_of_query = start_of_fragment;
      }

      size_t end_of_path = std::string::npos;
      size_t start_of_query = uri_text.find_first_of('?', carat);
      if (start_of_query != std::string::npos)
      {
	// Now we have the start of the query, and we also know if there's a
	// fragment, and where it starts.
        end_of_path = start_of_query;
        start_of_query++;
	m_query = uri_text.substr(start_of_query, (end_of_query - start_of_query));
      }

      // Now that we have the fragment and query out of the way, what remains is
      // the path.
      m_path = uri_text.substr(carat, (end_of_path - carat));
    }
  };

  std::string m_scheme;
  std::string m_username;
  std::string m_password;
  std::string m_domain;
  std::string m_path;
  std::string m_query;
  std::string m_fragment;

  std::map<std::string, std::string> m_query_dict;
  bool m_query_dict_initialized;
  
  unsigned long m_port;
};
