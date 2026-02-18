#include "../../includes/Request.hpp"
Request::Request() {
}

Request::~Request() {
}

// helper: get one line, remove \r\n

// helper: trim spaces from both sides

void Request::parse(const std::string &) {

	// --- 1. Parse Request Line ---
	// requestLine = "GET /index.html HTTP/1.1"

	// _method  = "GET"
	// _uri     = "/index.html"
	// _version = "HTTP/1.1"

	// --- 2. Parse Headers ---

	// --- 3. Parse Body ---
	// body only exists if Content-Length > 0
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

std::string Request::getHeader(const std::string &key) const {
	ConstHeaderIterator it = _headers.find(key);
	if (it == _headers.end())
		return "";
	return it->second;
}
