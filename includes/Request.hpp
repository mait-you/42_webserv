#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "head.hpp"

class Request {
  private:
	std::string						   _method;	 // GET, POST, DELETE
	std::string						   _uri;	 // requested path
	std::string						   _version; // HTTP/1.1
	std::map<std::string, std::string> _headers; // headers
	std::string						   _body;	 // request body

  public:
	Request();
	Request(const Request &other);
	Request &operator=(const Request &other);
	~Request();

	void parse(const std::string &rawRequest);

	std::string getMethod() const;
	std::string getUri() const;
	std::string getVersion() const;
	std::string getHeader(const std::string &key) const;
	std::string getBody() const;
};

#endif
