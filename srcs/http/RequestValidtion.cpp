#include "../../includes/Request.hpp"

bool Request::isValidVersion(const std::string &version) const {
	return version == HTTP_VERSION;
}

bool Request::isValidMethod(const std::string &method) const {
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool Request::isValidUri(const std::string &uri) const {
	if (uri.empty())
		return false;
	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127)
			return false;
	}
	return true;
}
