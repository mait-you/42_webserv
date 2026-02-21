#include "../../includes/Response.hpp"

	Response::Response()
	{

	}

	Response::Response(const Response &other)
	{
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_headers = other._headers;
	}
	Response &Response::operator=(const Response &other)
	{
		if (this != &other)
		{
			_statusCode = other._statusCode;
			_statusMessage = other._statusMessage;
			_headers = other._headers;
			_headers = other._headers;
		}
		return *this;
	}
	Response::~Response()
	{

	}

	void Response::setStatus(int code, const std::string &message)
	{

	}

	void Response::setHeader(const std::string &key, const std::string &value)
	{

	}

	void Response::setBody(const std::string &body)
	{

	}

	std::string Response::build() const
	{
		std::string fullResponse;
		fullResponse = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nhello\n";
		return fullResponse;
	}
