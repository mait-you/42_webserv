#include "../../includes/Request.hpp"

Request::Request() {
}

Request::Request(const Request &other)
	: _method(other._method), _uri(other._uri), _version(other._version),
	  _headers(other._headers), _body(other._body) {
}

Request &Request::operator=(const Request &other) {
	if (this != &other) {
		_method	 = other._method;
		_uri	 = other._uri;
		_version = other._version;
		_headers = other._headers;
		_body	 = other._body;
	}
	return *this;
}

Request::~Request() {
}

std::string Request::parseChunkedBody(const std::string &raw, std::size_t pos) {
	std::string body;

	while (pos < raw.size()) {
		std::string line = getLine(raw, pos);
		if (line.empty())
			break;

		std::size_t		   chunkSize = 0;
		std::istringstream iss(line);
		if (!(iss >> std::hex >> chunkSize) || chunkSize == 0)
			break;
		if (pos + chunkSize > raw.size())
			break;
		body += raw.substr(pos, chunkSize);
		pos += chunkSize;
		if (pos + 2 <= raw.size())
			pos += 2;
	}
	return body;
}

bool Request::parse(const std::string &buffer) {
	std::size_t		   pos	= 0;
	std::string		   line = getLine(buffer, pos);
	std::istringstream iss(line);
	if (!(iss >> _method >> _uri >> _version))
		return false;
	while (pos < buffer.size()) {
		std::string line = getLine(buffer, pos);
		if (line.empty())
			break;
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		_headers[trim(line.substr(0, colon))] = trim(line.substr(colon + 1));
	}
	std::string te = getHeader("Transfer-Encoding");
	if (te == "chunked") {
		_body = parseChunkedBody(buffer, pos);
	} else {
		std::string cl = getHeader("Content-Length");
		if (!cl.empty()) {
			std::size_t		   bodyLen = 0;
			std::istringstream iss(cl);
			if (!(iss >> bodyLen))
				return false;
			if (pos + bodyLen > buffer.size())
				return false;
			_body = buffer.substr(pos, bodyLen);
		}
	}
	return true;
}

std::string Request::getMethod() const {
	return _method;
}
std::string Request::getUri() const {
	return _uri;
}
std::string Request::getVersion() const {
	return _version;
}
std::string Request::getBody() const {
	return _body;
}

const Request::HeaderMap &Request::getHeaders() const {
	return _headers;
}

std::string Request::getHeader(const std::string &key) const {
	ConstHeaderIt it = _headers.find(key);
	return (it != _headers.end()) ? it->second : "";
}

std::ostream &operator<<(std::ostream &out, const Request &req) {
	out << req.getMethod() << " " << req.getUri() << " " << req.getVersion()
		<< "\n";

	const Request::HeaderMap &hdrs = req.getHeaders();
	for (Request::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		out << it->first << ": " << it->second << "\n";

	if (!req.getBody().empty())
		out << "\n" << req.getBody() << "\n";

	return out;
}
