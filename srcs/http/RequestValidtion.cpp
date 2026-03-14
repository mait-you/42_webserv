#include "../../includes/Request.hpp"

void Request::detectCgi() {
	if (_locConf && _locConf->has_cgi && _locConf->cgi.count(getExtension(_uri)))
		_hasCgi = true;
}

void Request::matchedLocation() {
	if (!_srvConf)
		return;

	const std::string& uri		  = cleanUri(_uri);
	std::size_t		   matchedLen = 0;

	for (size_t i = 0; i < _srvConf->locations.size(); i++) {
		const std::string& path = _srvConf->locations[i].path;

		if (uri.compare(0, path.size(), path) == 0) {
			if (path.size() > matchedLen) {
				matchedLen = path.size();
				_locConf   = &_srvConf->locations[i];
			}
		}
	}
}

bool Request::isValidVersion(const std::string& version) const {
	return version == HTTP_VERSION;
}

bool Request::isValidMethod(const std::string& method) const {
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool Request::isValidUri(const std::string& uri) const {
	if (uri.empty() || uri.size() > MAX_URI_LENGTH)
		return false;
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
