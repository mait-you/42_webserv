#include "../../includes/Response.hpp"

Response::Response() : _statusCode(200), _statusMessage("OK") {
}

Response::Response(const Response &other)
	: _statusCode(other._statusCode), _statusMessage(other._statusMessage),
	  _headers(other._headers), _body(other._body) {
}

Response &Response::operator=(const Response &other) {
	if (this != &other) {
		_statusCode	   = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers	   = other._headers;
		_body		   = other._body;
	}
	return *this;
}

Response::~Response() {
}

void Response::setStatus(int code, const std::string &message) {
	_statusCode	   = code;
	_statusMessage = message;
}

void Response::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void Response::setBody(const std::string &body) {
	_body = body;
	std::ostringstream oss;
	oss << body.size();
	_headers["Content-Length"] = oss.str();
}

std::string Response::build() const {
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

	for (std::map<std::string, std::string>::const_iterator it =
			 _headers.begin();
		 it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";

	oss << "\r\n" << _body;
	return oss.str();
}
