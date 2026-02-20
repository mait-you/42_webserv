#include "../../includes/Request.hpp"
Request::Request() {
}

Request::~Request() {
}

std::string Request::parseChunkedBody(const std::string &, std::size_t ) {
	std::string result;

	return result;
}

void Request::parse(const std::string &rawRequest) {
	std::size_t pos			= 0;
	std::string requestLine = getLine(rawRequest, pos);
	{
		std::istringstream iss(requestLine);
		iss >> _method >> _uri >> _version;
	}
	while (pos < rawRequest.size()) {
		std::string line = getLine(rawRequest, pos);
		if (line.empty())
			break;

		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue; // bad line, skip wiii3
		std::string key	  = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));
		_headers[key]	  = value;
	}
	HeaderIterator it = _headers.find("Content-Length");
	if (it != _headers.end()) {
		std::size_t bodyLen = 0;
		{
			std::istringstream iss(it->second.c_str());
			if (!(iss >> bodyLen))
				return;
		}
		if (pos + bodyLen <= rawRequest.size())
			_body = rawRequest.substr(pos, bodyLen);
	}
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

const Request::HeaderMap &Request::getHeaders() const {
	return _headers;
}

std::ostream &operator<<(std::ostream &out, const Request &request) {
	out << "Method:  " << request.getMethod() << "\n";
	out << "URI:     " << request.getUri() << "\n";
	out << "Version: " << request.getVersion() << "\n";

	const Request::HeaderMap	&headers = request.getHeaders();
	Request::ConstHeaderIterator it		 = headers.begin();
	while (it != headers.end()) {
		out << it->first << ": " << it->second << "\n";
		++it;
	}

	if (!request.getBody().empty())
		out << "\n" << request.getBody() << "\n";

	return out;
}
