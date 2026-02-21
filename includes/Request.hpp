#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "head.hpp"

class Request {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

  private:
	std::string _method;
	std::string _uri;
	std::string _version;
	HeaderMap	_headers;
	std::string _body;

  public:
	Request();
	Request(const Request &other);
	Request &operator=(const Request &other);
	~Request();

	// Returns true when parsing succeeds.
	bool parse(const std::string &buffer);

	std::string		 getMethod() const;
	std::string		 getUri() const;
	std::string		 getVersion() const;
	std::string		 getBody() const;
	const HeaderMap &getHeaders() const;
	std::string		 getHeader(const std::string &key) const;

  private:
	std::string parseChunkedBody(const std::string &raw, std::size_t pos);
};

std::ostream &operator<<(std::ostream &out, const Request &request);

#endif
