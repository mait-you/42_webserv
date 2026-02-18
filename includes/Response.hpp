#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "head.hpp"

class Response {
  private:
	int								   _statusCode;	   // 200, 404, etc.
	std::string						   _statusMessage; // OK, Not Found, etc.
	std::map<std::string, std::string> _headers;	   // response headers
	std::string						   _body;		   // response body

  public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &other);
	~Response();

	void setStatus(int code, const std::string &message);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);

	std::string build() const; // create full HTTP response
};

#endif
