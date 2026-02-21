#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "head.hpp"

class Response {
  private:
	int								   _statusCode;
	std::string						   _statusMessage;
	std::map<std::string, std::string> _headers;
	std::string						   _body;

  public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &other);
	~Response();

	void setStatus(int code, const std::string &message);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);

	std::string build() const;
};

#endif
