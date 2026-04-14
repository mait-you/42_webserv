#include "../../includes/Request.hpp"

void Request::detectCgi() {
	if (!_locConf || !_locConf->has_cgi)
		return;
	std::string cleanUri = resolveFullPath();

	for (std::map<std::string, std::string>::const_iterator it = _locConf->cgi.begin();
		 it != _locConf->cgi.end(); it++) {
		size_t pos = cleanUri.find(it->first);
		if (pos != std::string::npos) {
			size_t len = it->first.length();
			cleanUri   = cleanUri.substr(0, pos + len);
			break;
		}
	}

	if (_locConf->cgi.count(getExtension(cleanUri))) {
		_hasCgi = true;
	} else if (_locConf->cgi.count("." + getExtension(cleanUri))) {
		_hasCgi = true;
	}
	else if (_locConf->cgi.count("." + getExtension(cleanUri)))
	{
		_hasCgi = true;
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
	const std::string invalid = "\"<>\\^~`{}|";
	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127)
			return false;
		if (invalid.find(c) != std::string::npos)
			return false;
	}
	return true;
}

bool Request::isValidHeaders() const {
	const std::string& clHeader = getHeader("Content-Length");
	if (clHeader.empty())
		return true;
	long cl = std::atol(clHeader.c_str());
	if (cl < 0)
		return false;
	return true;
}
