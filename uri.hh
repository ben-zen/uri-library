// Copyright (C) 2015-2016 Ben Lewis <benjf5+github@gmail.com>
// Licensed under the MIT license.

#pragma once
#include <cctype>
#include <map>
#include <regex>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <cstdio>

class host
{
  /* The host component of a URI can be composed of a registered name, an IPv4
   * address, or an IP literal (IPv6 or future standards.) This implementation
   * will focus on IPv4 and IPv6 parsing (and registered names).
   */
public:
  enum class format
  {
    RegisteredName,
    InternetProtocolv4Address,
    InternetProtocolLiteral
  };
  
  host(char const *host_address, format host_format)
  {
    m_stored_format = host_format;
    switch (host_format)
    {
    case format::RegisteredName:
      m_registered_name = host_address;
      break;
    case format::InternetProtocolv4Address:
      parse_ipv4_address(std::string(host_address));
      break;
    case format::InternetProtocolLiteral:
      parse_ipv6_address(std::string(host_address));  
      break;
    }
  }
  
  ~host()
  {
  }
  
  format get_format()
  {
    return m_stored_format;
  };
  
  std::string to_string()
  {
    std::string formatted_name;
    switch (m_stored_format)
    {
    case format::RegisteredName:
      formatted_name = m_registered_name;
      break;
    case format::InternetProtocolv4Address:
      formatted_name = format_ipv4_address();
      break;
    case format::InternetProtocolLiteral:
      formatted_name = format_ipv6_address();
      break;
    }
    return formatted_name;
  };
  
private:

  std::string format_ipv4_address()
  {
    if (m_stored_format != format::InternetProtocolv4Address)
    {
      throw std::domain_error("format_ipv4_address should only be called for IPv4 hosts.");
    }
    
    std::string result = std::to_string(m_ipv4_address[0]) + "."
                         + std::to_string(m_ipv4_address[1]) + "."
                         + std::to_string(m_ipv4_address[2]) + "."
                         + std::to_string(m_ipv4_address[3]);
    return result;
  }

  std::string format_ipv6_address()
  {
    if (m_stored_format != format::InternetProtocolLiteral)
    {
      throw std::domain_error("format_ipv6_address should only be called for IPv6 hosts.");
    }

    enum class stanza_format
    {
      elision,
      printed_block
    };

    struct formatted_stanza
    {
      stanza_format format;
      union
      {
        unsigned short elided_stanzas;
        char stanza_buffer[5];
      };
    };

    size_t longest_elision = 0;
    std::vector<formatted_stanza> stanzas;
    // Format each stanza or find an elision, then iterate over the list again
    // to merge it into one string.
    for (size_t iter = 0; iter < 8; iter++)
    {
      if (m_ipv6_address[iter] == 0)
      {
        if (stanzas.empty() || (stanzas.back().format != stanza_format::elision))
        {
          formatted_stanza new_stanza = {};
          new_stanza.format = stanza_format::elision;
          stanzas.push_back(new_stanza);
        }

        formatted_stanza &elision_stanza = stanzas.back();
        elision_stanza.elided_stanzas++;
        if (elision_stanza.elided_stanzas > longest_elision)
        {
          longest_elision = elision_stanza.elided_stanzas;
        }
      }
      else
      {
        formatted_stanza new_stanza = {};
        new_stanza.format = stanza_format::printed_block;
        std::snprintf(new_stanza.stanza_buffer, 5, "%hx", m_ipv6_address[iter]);
        stanzas.push_back(new_stanza);
      }
    }

    std::string result;
    bool already_elided = false;
    for (auto stanza = stanzas.begin(); stanza != stanzas.end(); stanza++)
    {
      if (stanza->format == stanza_format::elision)
      {
        if ((stanza->elided_stanzas == longest_elision) && !already_elided)
        {
          already_elided = true;
          // Only put a colon in if there won't be one already.
          if (stanza == stanzas.begin())
          {
            result.push_back(':');
          }

          result.push_back(':');
        }
        else
        {
          // Otherwise, we fill in the number of elided stanzas.
          unsigned short stanza_count = stanza->elided_stanzas;
          while (stanza_count > 0)
          {
            result.push_back('0');
            stanza_count--;
            if (stanza_count != 0)
            {
              result.push_back(':');
            }
          }

          if ((stanza + 1) != stanzas.end())
          {
            result.push_back(':');
          }
        }
      }
      else
      {
        result.append(stanza->stanza_buffer);
        if ((stanza + 1) != stanzas.end())
        {
          result.push_back(':');
        }
      }
    }
    return result;
  }
      
  void parse_ipv4_address(std::string const &address)
  {
    // An IPv4 address is in the form `xxx.xxx.xxx.xxx`, where each stanza is
    // a decimal value between 0 and 255. Parsing this is reasonably direct.
    
    // There is a shorter form of this, but for the moment I'll let it slide as
    // a first pass.
    std::regex ipv4_address_format("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
    if (!std::regex_match(address, ipv4_address_format))
    {
      throw std::invalid_argument("Supplied host name is not an IPv4 address. Supplied address was \""
                                  + address + "\".");
    }
    
    std::string match_address = address;
    std::regex ipv4_stanza_format("[0-9]{1,3}");
    std::regex_iterator<std::string::iterator> ipv4_stanzas(match_address.begin(),
                                                            match_address.end(),
                                                            ipv4_stanza_format);
    std::regex_iterator<std::string::iterator> iter_end;
 
    for (int iter = 0; iter < 4; iter++)
    {
      if (ipv4_stanzas == iter_end)
      {
        throw std::logic_error("Unexpected end of IPv4 address found while parsing address: "
                               + address);
      }
      int stanza = std::stoi((*ipv4_stanzas).str());
      if (stanza > 255)
      {
        throw std::invalid_argument("Supplied string is not an IPv4 address: " + address
                                    + "\n" + "The stanza \"" + (*ipv4_stanzas).str()
                                    + "\" does not fit in a byte.");
      }
      else
      {
        // Since the match had to be specifically 1-3 digits, it cannot be negative.
        m_ipv4_address[iter] = static_cast<unsigned char>(stanza);
      }
      ipv4_stanzas++;
    }
  }
  
  void parse_ipv6_address(std::string const &address)
  {
    // Note that this format stage simply confirms that the address consists
    // only stanzas of 0-4 hexadecimal digits divided by colons, and there
    // are no other characters present. Actual comprehension of the address
    // is left to a later stage.
    std::regex ipv6_address_rough_format("[0-9A-Fa-f]{0,4}:([0-9A-Fa-f]{0,4}:){1,6}[0-9A-Fa-f]{0,4}");
    if (!std::regex_match(address, ipv6_address_rough_format))
    {
      throw std::invalid_argument("Supplied hostname is not an IPv6 address. Supplied address was \""
      + address + "\".");
    }
    
    // Now we know the address is roughly in the right form. At this point we
    // can start considering how to parse each individual component.
    
    enum class stanza_type
    {
      address_piece,
      elision
    };
 
    // if format is stanza_type::elision, address_piece will be 0.
    struct parse_object
    {
      stanza_type format;
      unsigned short address_piece;
    };
      

    std::vector<parse_object> components;
    // Take an iterator to the start of the address, then iterate through
    // the string, finding the end of each stanza (or finding an elision).
    std::string::const_iterator cursor = address.begin();
    while (cursor != address.end())
    {
      if (*cursor == ':')
      {
        cursor++;
        if (cursor != address.end())
        {
          if (*cursor == ':')
          {
            parse_object component = {};
            component.format = stanza_type::elision;
            components.push_back(component);
            cursor++;
          }
          else if (components.empty())
          {
            throw std::invalid_argument("Missing first stanza in address: \""
                                        + address + "\".");
          }
        }
        else
        {
          throw std::invalid_argument("Unexpected end of address found while "
                                      "parsing: \"" + address + "\".");
        }
      }
      else
      {
        // Some character that is not a colon, at most four. Build a lookahead
        // cursor, grab characters that are alphanumeric, and then parse an int
        // that's 16 bits at most. If it's more, it's not valid and throw an
        // exception. You can also check that a character is a hexadecimal 
        // digit here.
            
        std::string piece;
        int index = 0;
        while ((index < 4) && (cursor != address.end()) && (*cursor != ':'))
        {
          if (!std::isxdigit(*cursor))
          {
            throw std::invalid_argument("Non-hexadecimal character encountered in parsing."
                                        "\nThe provided string was: \"" + address
                                        + "\".");
          }
              
          piece.push_back(*cursor);
          index++;
          cursor++;
        }
            
        if ((index == 4) && (cursor != address.end()) && (*cursor != ':'))
        {
          throw std::invalid_argument("Non-hexadecimal character encountered in parsing."
                                      "\nThe provided string was: \"" + address
                                      + "\".");
        }
            
        int parsed_piece = std::stoi(piece, nullptr, 16);
        if (parsed_piece > 0xFFFF)
        {
          throw std::domain_error("Attempted to cast a value larger than "
                                  "an unsigned short to an unsigned short.");
        }
  
        parse_object component = {};
        component.format = stanza_type::address_piece;
        component.address_piece = static_cast<unsigned short>(parsed_piece);
        components.push_back(component);
      }
    }
        
    bool after_elision = false;
    int stanza_index = 0;
    for (auto iter = components.begin(); iter != components.end(); iter++)
    {
      if (stanza_index > 7)
      {
        throw std::logic_error("Attempting to write to a stanza outside an IPv6"
                               " address.");
      }

      if (iter->format == stanza_type::elision)
      {
        if (after_elision)
        {
          throw std::invalid_argument("More than one elision encountered while parsing "
                                      "this IPv6 address: \"" + address + "\".");
        }

        // This line is a little odd, but the effect is basically to move the
        // index to where we'd end the elision, and then at the end of the loop
        // we're going to move ahead another index and exit the loop.
        stanza_index = 8 - (components.end() - iter);
        after_elision = true;
      }
      else
      {
        m_ipv6_address[stanza_index] = iter->address_piece;
      }
      stanza_index++;
    }
  }

  format m_stored_format;
  std::string    m_registered_name;
  unsigned char  m_ipv4_address[4] = {};
  unsigned short m_ipv6_address[8] = {};
};

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
   * scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
   */

public:

  enum class scheme_category
  {
    Hierarchical,
    NonHierarchical
  };

  enum class component
  {
    Scheme,
    Content,
    Username,
    Password,
    Host,
    Port,
    Path,
    Query,
    Fragment
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

  uri(std::map<component, std::string> const &components, scheme_category category, bool rooted_path) :
    m_category(category),
    m_path_is_rooted(rooted_path)
  {
    if (components.count(component::Scheme))
    {
      if (components.at(component::Scheme).length() == 0)
      {
        throw std::invalid_argument("Scheme cannot be empty.");
      }
      m_scheme = components.at(component::Scheme);
    }
    else
    {
      throw std::invalid_argument("A URI must have a scheme.");
    }

    if (category == scheme_category::Hierarchical)
    {
      if (components.count(component::Content))
      {
	      throw std::invalid_argument("The content component is only for use in non-hierarchical URIs.");
      }

      bool has_username = components.count(component::Username);
      bool has_password = components.count(component::Password);
      if (has_username && has_password)
      {
        m_username = components.at(component::Username);
        m_password = components.at(component::Password);
      }
      else if ((has_username && !has_password) || (!has_username && has_password))
      {
	      throw std::invalid_argument("If a username or password is supplied, both must be provided.");
      }

      if (components.count(component::Host))
      {
        m_host = components.at(component::Host);
      }

      if (components.count(component::Port))
      {
        m_port = std::stoul(components.at(component::Port));
      }

      if (components.count(component::Path))
      {
        m_path = components.at(component::Path);
      }
      else
      {
        throw std::invalid_argument("A path is required on a hierarchical URI, even an empty path.");
      }
    }
    else
    {
      if (components.count(component::Username)
         || components.count(component::Password)
         || components.count(component::Host)
         || components.count(component::Port)
         || components.count(component::Path))
      {
        throw std::invalid_argument("None of the hierarchical components are allowed in a non-hierarchical URI.");
      }

      if (components.count(component::Content))
      {
        m_content = components.at(component::Content);
      }
      else
      {
        throw std::invalid_argument("Content is a required component for a non-hierarchical URI, even an empty string.");
      }
    }

    if (components.count(component::Query))
    {
      m_query = components.at(component::Query);
    }

    if (components.count(component::Fragment))
    {
      m_fragment = components.at(component::Fragment);
    } 
  } 

  uri(uri const &other, std::map<component, std::string> const &replacements) :
    m_category(other.m_category),
    m_path_is_rooted(other.m_path_is_rooted)
  {
    m_scheme = (replacements.count(component::Scheme))
      ? replacements.at(component::Scheme) : other.m_scheme;

    if (m_category == scheme_category::Hierarchical)
    {
      m_username = (replacements.count(component::Username))
        ? replacements.at(component::Username) : other.m_username;

      m_password = (replacements.count(component::Password))
        ? replacements.at(component::Password) : other.m_password;

      m_host = (replacements.count(component::Host))
        ? replacements.at(component::Host) : other.m_host;

      m_port = (replacements.count(component::Port))
        ? std::stoul(replacements.at(component::Port)) : other.m_port;

      m_path = (replacements.count(component::Path))
        ? replacements.at(component::Path) : other.m_path;
    }
    else
    {
      m_content = (replacements.count(component::Content))
        ? replacements.at(component::Content) : other.m_content;
    }

    m_query = (replacements.count(component::Query))
      ? replacements.at(component::Query) : other.m_query;

    m_fragment = (replacements.count(component::Fragment))
      ? replacements.at(component::Fragment) : other.m_fragment;
  }

  // Copy constructor; just use the copy assignment operator internally.
  uri(uri const &other)
  {
    *this = other;
  };

  // Copy assignment operator
  uri &operator=(uri const &other)
  {
    if (this != &other)
    {
      m_scheme = other.m_scheme;
      m_content = other.m_content;
      m_username = other.m_username;
      m_password = other.m_password;
      m_host = other.m_host;
      m_path = other.m_path;
      m_query = other.m_query;
      m_fragment = other.m_fragment;
      m_query_dict = other.m_query_dict;
      m_category = other.m_category;
      m_port = other.m_port;
      m_path_is_rooted = other.m_path_is_rooted;
    }
    return *this;
  }

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
    size_t const uri_length = uri_text.length();

    if (uri_length == 0)
    {
      throw std::invalid_argument("URIs cannot be of zero length.");
    }

    std::string::const_iterator cursor = parse_scheme(uri_text, 
                                                      uri_text.begin());
    // After calling parse_scheme, *cursor == ':'; none of the following parsers
    // expect a separator character, so we advance the cursor upon calling them.
    cursor = parse_content(uri_text, (cursor + 1));

    if ((cursor != uri_text.end()) && (*cursor == '?'))
    {
      cursor = parse_query(uri_text, (cursor + 1));
    }

    if ((cursor != uri_text.end()) && (*cursor == '#'))
    {
      cursor = parse_fragment(uri_text, (cursor + 1));
    }

    init_query_dictionary(); // If the query string is empty, this will be empty too.

  };

  std::string::const_iterator parse_scheme(std::string const &uri_text,
					   std::string::const_iterator scheme_start)
  {
    std::string::const_iterator scheme_end = scheme_start;
    while ((scheme_end != uri_text.end()) && (*scheme_end != ':'))
    {
      if (!(isalnum(*scheme_end) || (*scheme_end == '-')
	    || (*scheme_end == '+') || (*scheme_end == '.')))
      {
        throw std::invalid_argument("Invalid character found in the scheme component. Supplied URI was: \""
          + uri_text + "\".");
      }
      ++scheme_end;
    }

    if (scheme_end == uri_text.end())
    {
      throw std::invalid_argument("End of URI found while parsing the scheme. Supplied URI was: \""
        + uri_text + "\".");
    }

    if (scheme_start == scheme_end)
    {
      throw std::invalid_argument("Scheme component cannot be zero-length. Supplied URI was: \""
				  + uri_text + "\".");
    }

    m_scheme = std::move(std::string(scheme_start, scheme_end));
    return scheme_end;
  };

  std::string::const_iterator parse_content(std::string const &uri_text,
					    std::string::const_iterator content_start)
  {
    std::string::const_iterator content_end = content_start;
    while ((content_end != uri_text.end()) && (*content_end != '?') && (*content_end != '#'))
    {
      ++content_end;
    }

    m_content = std::move(std::string(content_start, content_end));

    if ((m_category == scheme_category::Hierarchical) && (m_content.length() > 0))
    {
      // If it's a hierarchical URI, the content should be parsed for the hierarchical components.
      std::string::const_iterator path_start = m_content.begin();
      std::string::const_iterator path_end = m_content.end();
      if (!m_content.compare(0, 2, "//"))
      {
	// In this case an authority component is present.
	std::string::const_iterator authority_cursor = (m_content.begin() + 2);
	if (m_content.find_first_of('@') != std::string::npos)
	{
	  std::string::const_iterator userpass_divider = parse_username(uri_text,
									m_content,
									authority_cursor);
	  authority_cursor = parse_password(uri_text, m_content, (userpass_divider + 1));
	  // After this call, *authority_cursor == '@', so we skip over it.
	  ++authority_cursor;
	}

	authority_cursor = parse_host(uri_text, m_content, authority_cursor);

	if (*authority_cursor == ':')
	{
	  authority_cursor = parse_port(uri_text, m_content, (authority_cursor + 1));
	}

	if (*authority_cursor == '/')
	{
	  // Then the path is rooted, and we should note this.
	  m_path_is_rooted = true;
	  path_start = authority_cursor + 1;
	}
      }
      else if (!m_content.compare(0, 1, "/"))
      {
	m_path_is_rooted = true;
	++path_start;
      }

      // We can now build the path based on what remains in the content string,
      // since that's all that exists after the host and optional port component.
      m_path = std::move(std::string(path_start, path_end));
    }
    return content_end;
  };

  std::string::const_iterator parse_username(std::string const &uri_text,
					     std::string const &content,
					     std::string::const_iterator username_start)
  {
    std::string::const_iterator username_end = username_start;
    // Since this is only reachable when '@' was in the content string, we can
    // ignore the end-of-string case.
    while (*username_end != ':')
    {
      if (*username_end == '@')
      {
	throw std::invalid_argument("Username must be followed by a password. Supplied URI was: \""
				    + uri_text + "\".");
      }
      ++username_end;
    }
    m_username = std::move(std::string(username_start, username_end));
    return username_end;
  };

  std::string::const_iterator parse_password(std::string const &uri_text,
					     std::string const &content,
					     std::string::const_iterator password_start)
  {
    std::string::const_iterator password_end = password_start;
    while (*password_end != '@')
    {
      ++password_end;
    }

    m_password = std::move(std::string(password_start, password_end));
    return password_end;
  };

  std::string::const_iterator parse_host(std::string const &uri_text,
					 std::string const &content,
					 std::string::const_iterator host_start)
  {
    std::string::const_iterator host_end = host_start;
    // So, the host can contain a few things. It can be a domain, it can be an
    // IPv4 address, it can be an IPv6 address, or an IPvFuture literal. In the
    // case of those last two, it's of the form [...] where what's between the
    // brackets is a matter of which IPv?? version it is.
    while (host_end != content.end())
    {
      if (*host_end == '[')
      {
	// We're parsing an IPv6 or IPvFuture address, so we should handle that
	// instead of the normal procedure.
	while ((host_end != content.end()) && (*host_end != ']'))
	{
	  ++host_end;
	}

	if (host_end == content.end())
	{
	  throw std::invalid_argument("End of content component encountered "
				      "while parsing the host component. "
				      "Supplied URI was: \""
				      + uri_text + "\".");
	}

	++host_end;
	break;
	// We can stop looping, we found the end of the IP literal, which is the
	// whole of the host component when one's in use.
      }
      else if ((*host_end == ':') || (*host_end == '/'))
      {
	break;
      }
      else
      {
	++host_end;
      }
    }

    m_host = std::move(std::string(host_start, host_end));
    return host_end;
  };

  std::string::const_iterator parse_port(std::string const &uri_text,
					std::string const &content,
					std::string::const_iterator port_start)
  {
    std::string::const_iterator port_end = port_start;
    while ((port_end != content.end()) && (*port_end != '/'))
    {
      if (!isdigit(*port_end))
      {
	throw std::invalid_argument("Invalid character while parsing the port. "
				    "Supplied URI was: \"" + uri_text + "\".");
      }

      ++port_end;
    }

    m_port = std::stoul(std::string(port_start, port_end));
    return port_end;
  };

  std::string::const_iterator parse_query(std::string const &uri_text,
                                          std::string::const_iterator query_start)
  {
    std::string::const_iterator query_end = query_start;
    while ((query_end != uri_text.end()) && (*query_end != '#'))
    {
      // Queries can contain almost any character except hash, which is reserved
      // for the start of the fragment.
      ++query_end;
    }
    m_query = std::move(std::string(query_start, query_end));
    return query_end;
  };

  std::string::const_iterator parse_fragment(std::string const &uri_text,
                                             std::string::const_iterator fragment_start)
  {
    m_fragment = std::move(std::string(fragment_start, uri_text.end()));
    return uri_text.end();
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
