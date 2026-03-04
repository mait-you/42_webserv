#include "../../includes/Request.hpp"

bool Request::isValidVersion(const std::string& version) const {
	return version == HTTP_VERSION;
}

bool Request::isValidMethod(const std::string& method) const {
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool Request::isValidUri(const std::string& uri) const {
	if (uri.empty() || uri.size() > MAX_URI_LENGTH)
		return false;
	// if (uri[0] != '/' && uri.substr(0, 7) != "http://")
	// 	return false;
	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127)
			return false;
		if (c == '"' || c == '<' || c == '>' || c == '\\' || c == '^' || c == '`' || c == '{'
			|| c == '}' || c == '|')
			return false;
	}
	return true;
}

bool Request::isValidHeaders() const {
	if (getMethod() == "POST" && getHeader("Content-Length").empty())
		return false;
	return true;
}
