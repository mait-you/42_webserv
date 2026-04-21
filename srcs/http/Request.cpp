#include "../../includes/http/Request.hpp"

#include "../../includes/utils/Utils.hpp"

Request::Request()
		: HttpStatus(), _srvConf(NULL), _locConf(NULL), _contentLength(0),
		  _parseState(PARSE_REQUEST_LINE), _hasCgi(false) {}

Request::Request(const ServerConfig* srvConf, const std::string& clientIp)
		: HttpStatus(), _srvConf(srvConf), _locConf(NULL), _clientIp(clientIp), _contentLength(0),
		  _parseState(PARSE_REQUEST_LINE), _hasCgi(false) {}

Request::Request(const Request& other)
		: HttpStatus(other), _srvConf(other._srvConf), _locConf(other._locConf),
		  _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _clientIp(other._clientIp),
		  _contentLength(other._contentLength), _parseState(other._parseState),
		  _hasCgi(other._hasCgi) {}

Request& Request::operator=(const Request& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_srvConf	   = other._srvConf;
		_locConf	   = other._locConf;
		_method		   = other._method;
		_uri		   = other._uri;
		_version	   = other._version;
		_headers	   = other._headers;
		_body		   = other._body;
		_clientIp	   = other._clientIp;
		_contentLength = other._contentLength;
		_parseState	   = other._parseState;
		_hasCgi		   = other._hasCgi;
	}
	return *this;
}

Request::~Request() {}

bool Request::parse(std::string& buffer) {
	while (_parseState == PARSE_REQUEST_LINE || _parseState == PARSE_HEADERS) {
		/* RFC 1945 §2.2 — lines end with CRLF */
		std::size_t pos = buffer.find("\r\n");
		if (pos == std::string::npos)
			break;

		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		processLine(line);
	}
	if (_parseState == PARSE_BODY) {
		if (buffer.size() >= _contentLength) {
			_body = buffer.substr(0, _contentLength);
			buffer.erase(0, _contentLength);
			_parseState = PARSE_DONE;
		}
	}
	return true;
}

void Request::processLine(const std::string& line) {
	if (_parseState == PARSE_REQUEST_LINE) {
		return parseRequestLine(line);
	}
	if (_parseState == PARSE_HEADERS) {
		if (line.empty()) {
			std::string cl = getHeader("content-length");
			if (!cl.empty()) {
				_contentLength = static_cast<std::size_t>(std::atol(cl.c_str()));
				_parseState	   = (_contentLength > 0) ? PARSE_BODY : PARSE_DONE;
				if (_contentLength > _srvConf->client_max_body_size)  // serve side
					return setError(HTTP_400_BAD_REQUEST);
			} else {
				_contentLength = 0;
				_parseState	   = PARSE_DONE;
			}
		} else
			parseHeaderLine(line);
	}
}

void Request::parseRequestLine(const std::string& line) {
	std::istringstream ss(line);

	ss >> _method >> _uri;
	if (_method.empty() || _uri.empty())
		return setError(HTTP_400_BAD_REQUEST);

	/* RFC 1945 §5.1.1 — unsupported method */
	if (_method != "GET" && _method != "POST" && _method != "DELETE")
		return setError(HTTP_501_NOT_IMPLEMENTED);

	/* RFC 1945 §3.2 — validate URI characters */
	if (!isValidUri(_uri))
		return setError(HTTP_400_BAD_REQUEST);

	/* RFC 1945 §3.1 — only full HTTP/x.x requests accepted */
	ss >> _version;
	if (_version.empty())
		_httpVersion = HTTP_0_9;
	else if (_version == "HTTP/1.0")
		_httpVersion = HTTP_1_0;
	else if (_version == "HTTP/1.1")
		_httpVersion = HTTP_1_1;
	else
		return setError(HTTP_400_BAD_REQUEST);
	if (_httpVersion == HTTP_0_9 && _method != "GET")
		return setError(HTTP_400_BAD_REQUEST);

	std::string extra;
	if (ss >> extra)
		return setError(HTTP_400_BAD_REQUEST);

	/* Match URI to a configured location block. */
	if (!matchLocation())
		return setError(HTTP_404_NOT_FOUND);

	detectCgi();
	_parseState = PARSE_HEADERS;
}

void Request::parseHeaderLine(const std::string& line) {
	/* RFC 1945 §4 —  Simple-Request do not allow the use of any header */
	if (_httpVersion == HTTP_0_9)
		return setError(HTTP_400_BAD_REQUEST);
	std::size_t colon = line.find(':');
	if (colon == std::string::npos)
		return setError(HTTP_400_BAD_REQUEST);

	std::string name  = line.substr(0, colon);
	std::string value = trimStr(line.substr(colon + 1));

	if (name.empty())
		return setError(HTTP_400_BAD_REQUEST);

	/* RFC 1945 §4.2 — no LWS between field-name and ":" */
	if (std::isspace((unsigned char) name[name.size() - 1]))
		return setError(HTTP_400_BAD_REQUEST);

	/* RFC 1945 §2.2 — token: printable US-ASCII (33-126), no CTL */
	for (std::size_t i = 0; i < name.size(); ++i) {
		unsigned char c = name[i];
		if (c < 33 || c == 127)
			return setError(HTTP_400_BAD_REQUEST);
	}

	_headers[toLower(name)] = value;
}

bool Request::isValidUri(const std::string& uri) const {
	if (uri.empty() || uri.size() > MAX_URI_LENGTH)
		return false;

	/* Characters that are never valid in a Request-URI */
	const std::string forbidden = "\"<>\\^~`{}|";

	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127) /* CTL or DEL */
			return false;
		if (forbidden.find(c) != std::string::npos)
			return false;
	}
	return true;
}

bool Request::matchLocation() {
	if (!_srvConf)
		return false;

	const std::string& uri	   = resolvePath();
	std::size_t		   bestLen = 0;

	for (std::size_t i = 0; i < _srvConf->locations.size(); ++i) {
		const std::string& locPath = _srvConf->locations[i].path;

		if (uri.compare(0, locPath.size(), locPath) != 0)
			continue;

		if (locPath != "/" && uri.size() != locPath.size() && uri[locPath.size()] != '/')
			continue;

		if (locPath.size() > bestLen) {
			bestLen	 = locPath.size();
			_locConf = &_srvConf->locations[i];
		}
	}

	return _locConf != NULL;
}

void Request::detectCgi() {
	if (!_locConf || !_locConf->has_cgi)
		return;

	std::string path = resolveFullPath();

	for (std::map<std::string, std::string>::const_iterator it = _locConf->cgi.begin();
		 it != _locConf->cgi.end(); ++it) {
		std::size_t pos = path.find(it->first);
		if (pos != std::string::npos) {
			path = path.substr(0, pos + it->first.size());
			break;
		}
	}

	std::string ext = getExtension(path); /* e.g. "pl" */

	if (_locConf->cgi.count(ext) || _locConf->cgi.count("." + ext))
		_hasCgi = true;
}

void Request::setError(CodeStatus code) {
	setStatus(code);
	_parseState = PARSE_ERROR;
}

/*
 * Example: "/a/../b?x=1" → "/b"
 */
std::string Request::resolvePath() const {
	if (_uri.empty())
		return "/";
	std::string path = _uri;
	std::size_t q	 = path.find('?');
	if (q != std::string::npos)
		path = path.substr(0, q);

	std::vector<std::string> parts;
	std::stringstream		 ss(path);
	std::string				 seg;

	while (std::getline(ss, seg, '/')) {
		if (seg.empty() || seg == ".")
			continue;
		if (seg == "..") {
			if (!parts.empty())
				parts.pop_back();
		} else {
			parts.push_back(seg);
		}
	}

	std::string clean;
	for (std::size_t i = 0; i < parts.size(); ++i)
		clean += "/" + parts[i];

	if (clean.empty() || _uri[_uri.size() - 1] == '/')
		clean += "/";

	return clean;
}

/*
 * Example (normal): root="/var/www", URI="/img/a.png" → "/var/www/img/a.png"
 * Example (alias):  root="/data",   loc="/img", URI="/img/a.png" → "/data/a.png"
 */
std::string Request::resolveFullPath() const {
	const std::string& root	   = !_locConf->root.empty() ? _locConf->root : _srvConf->root;
	std::string		   relPath = resolvePath();
	const std::string& locPath = _locConf->path;

	if (_locConf->isAlias) {
		if (locPath != "/" && relPath.compare(0, locPath.size(), locPath) == 0
			&& (relPath.size() == locPath.size() || relPath[locPath.size()] == '/')) {
			relPath = relPath.substr(locPath.size());
			if (relPath.empty())
				relPath = "/";
		}
	}

	if (_hasCgi) {
		for (std::map<std::string, std::string>::const_iterator it = _locConf->cgi.begin();
			 it != _locConf->cgi.end(); ++it) {
			std::size_t pos = relPath.find(it->first);
			if (pos != std::string::npos) {
				relPath = relPath.substr(0, pos + it->first.size());
				break;
			}
		}
	}

	bool rootSlash = (root[root.size() - 1] == '/');
	bool relSlash  = (!relPath.empty() && relPath[0] == '/');

	if (rootSlash && relSlash)
		return root + relPath.substr(1);
	if (!rootSlash && !relSlash)
		return root + "/" + relPath;
	return root + relPath;
}

bool Request::isComplete() const {
	return (_parseState == PARSE_DONE || _parseState == PARSE_ERROR);
}

bool Request::isValid() const {
	return isSuccess();
}

bool Request::hasCgi() const {
	return _hasCgi;
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
std::string Request::getClientIp() const {
	return _clientIp;
}

const Request::HeaderMap& Request::getHeaders() const {
	return _headers;
}

const LocationConfig* Request::getLocationConf() const {
	return _locConf;
}
const ServerConfig* Request::getConf() const {
	return _srvConf;
}

std::string Request::getHeader(const std::string& key) const {
	ConstHeaderIt it = _headers.find(toLower(key));
	return (it != _headers.end()) ? it->second : "";
}
