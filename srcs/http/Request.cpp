#include "../../includes/Request.hpp"

Request::Request() : _error(OK) {
}

Request::Request(const Request &other)
	: _method(other._method), _uri(other._uri), _version(other._version),
	  _headers(other._headers), _body(other._body), _error(other._error) {
}

Request &Request::operator=(const Request &other) {
	if (this != &other) {
		_method	 = other._method;
		_uri	 = other._uri;
		_version = other._version;
		_headers = other._headers;
		_body	 = other._body;
		_error	 = other._error;
	}
	return *this;
}

Request::~Request() {
}

bool Request::isValidMethod(const std::string &method) const {
	return (method == "GET" || method == "POST" || method == "DELETE");
}

bool Request::checkRequestComplete(const std::string &recvBuffer) const {
	std::size_t headerEnd = recvBuffer.find(END_OF_HEADERS);
	if (headerEnd == std::string::npos)
		return false;
	std::string headers = recvBuffer.substr(0, headerEnd);
	if (headers.find("Transfer-Encoding: chunked") != std::string::npos)
		return recvBuffer.find("0" END_OF_HEADERS) != std::string::npos;
	std::size_t clPos = headers.find("Content-Length: ");
	if (clPos != std::string::npos) {
		std::size_t		   lineEnd = headers.find("\r\n", clPos + 16);
		std::size_t		   bodyLen = 0;
		std::istringstream iss(
			headers.substr(clPos + 16, lineEnd - (clPos + 16)));
		iss >> bodyLen;
		return (recvBuffer.size() - (headerEnd + 4)) >= bodyLen;
	}
	return true;
}

bool Request::isValidUriChars(const std::string &uri) const {
	if (uri.empty())
		return false;
	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127)
			return false;
	}
	return true;
}

void Request::validate() {
	if (!isValidMethod(_method)) {
		_error = BAD_REQUEST;
		return;
	}
	if (!isValidUriChars(_uri)) {
		_error = BAD_REQUEST;
		return;
	}
	if (_version != HTTP_VERSION) {
		_error = UNSUPPORTED_VERSION;
		return;
	}
	if (getHeader("Host").empty()) {
		_error = BAD_REQUEST;
		return;
	}
	_error = OK;
}

bool Request::parse(const std::string &recvBuffer) {
	if (!checkRequestComplete(recvBuffer))
		return false;
	_error					= OK;
	std::size_t		   pos	= 0;
	std::string		   line = getLine(recvBuffer, pos);
	std::istringstream iss(line);
	if (!(iss >> _method >> _uri >> _version))
		return false;
	while (pos < recvBuffer.size()) {
		std::string hline = getLine(recvBuffer, pos);
		if (hline.empty())
			break;
		std::size_t colon = hline.find(':');
		if (colon == std::string::npos)
			continue;
		_headers[trim(hline.substr(0, colon))] = trim(hline.substr(colon + 1));
	}
	if (getHeader("Transfer-Encoding") == "chunked") {
		_body = parseChunkedBody(recvBuffer, pos);
	} else {
		std::string clStr = getHeader("Content-Length");
		if (!clStr.empty()) {
			std::size_t		   bodyLen = 0;
			std::istringstream iss2(clStr);
			if (!(iss2 >> bodyLen))
				return false;
			if (pos + bodyLen > recvBuffer.size())
				return false;
			_body = recvBuffer.substr(pos, bodyLen);
		}
	}
	return true;
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

Request::HttpError Request::getError() const {
	return _error;
}
bool Request::isValid() const {
	return _error == OK;
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
