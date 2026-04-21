#include "../../includes/http/Request.hpp"

#include "../../includes/utils/Utils.hpp"

Request::Request()
		: HttpStatus(), _srvConf(NULL), _locConf(NULL), _state(PARSE_REQUEST_LINE),
		  _parsePos(0), _hasCgi(false) {}

Request::Request(const Socket* servSocket, const std::string& clientIp)
		: HttpStatus(), _srvConf(servSocket->getConf()), _locConf(NULL),
		  _clientIp(clientIp), _serverPort(servSocket->getPort()), _serverIp(servSocket->getIp()), _state(PARSE_REQUEST_LINE), _parsePos(0), _hasCgi(false) {}

Request::Request(const Request& other)
		: HttpStatus(other), _srvConf(other._srvConf), _locConf(other._locConf),
		  _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _clientIp(other._clientIp),
		  _serverPort(other._serverPort), _serverIp(other._serverIp),
		  _state(other._state), _parsePos(other._parsePos), _hasCgi(other._hasCgi) {}

Request& Request::operator=(const Request& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_srvConf  = other._srvConf;
		_locConf  = other._locConf;
		_method	  = other._method;
		_uri	  = other._uri;
		_version  = other._version;
		_headers  = other._headers;
		_body	  = other._body;
		_clientIp = other._clientIp;
		_serverPort = other._serverPort;
		_serverIp = other._serverIp;
		_state	  = other._state;
		_parsePos = other._parsePos;
		_hasCgi	  = other._hasCgi;
	}
	return *this;
}

Request::~Request() {}

bool Request::matchedLocation() {
	if (!_srvConf)
		return false;
	const std::string& uri		  = resolvePath();
	std::size_t		   matchedLen = 0;
	for (size_t i = 0; i < _srvConf->locations.size(); i++) {
		const std::string& path = _srvConf->locations[i].path;
		if (uri.compare(0, path.size(), path) == 0) {
			if (path == "/" || uri.size() == path.size() || uri[path.size()] == '/') {
				if (path.size() > matchedLen) {
					matchedLen = path.size();
					_locConf   = &_srvConf->locations[i];
				}
			}
		}
	}
	if (!_locConf)
		return false;
	return true;
}

bool Request::parseRequestLine(const std::string& buf) {
	std::string line;
	if (!getLine(buf, _parsePos, line))
		return false;

	std::istringstream iss(line);
	if (!(iss >> _method >> _uri >> _version))
		return setError(HTTP_400_BAD_REQUEST);
	std::string extra;
	if (iss >> extra)
		return setError(HTTP_400_BAD_REQUEST);
	if (!isValidMethod(_method))
		return setError(HTTP_501_NOT_IMPLEMENTED);
	if (!isValidUri(_uri))
		return setError(HTTP_400_BAD_REQUEST);
	// if (!isValidVersion(_version))
	// 	return setError(HTTP_400_BAD_REQUEST);
	if (!matchedLocation())
		return setError(HTTP_400_BAD_REQUEST);
	detectCgi();
	setParseState(PARSE_HEADERS);
	return true;
}

bool Request::parseHeaders(const std::string& buf) {
	while (true) {
		std::string line;
		if (!getLine(buf, _parsePos, line))
			return false;
		if (line.empty()) {
			if (!isValidHeaders())
				return setError(HTTP_400_BAD_REQUEST);
			setParseState(PARSE_BODY);
			return true;
		}
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			return setError(HTTP_400_BAD_REQUEST);
		std::string key	  = toLower(trimStr(line.substr(0, colon)));
		std::string value = trimStr(line.substr(colon + 1));
		if (key.empty())
			return setError(HTTP_400_BAD_REQUEST);
		if (key == "content-length" && _headers.count(key))
			return setError(HTTP_400_BAD_REQUEST);
		_headers[key] = value;
	}
}

bool Request::parseBody(const std::string& buf) {
	std::string clStr = getHeader("content-length");
	if (clStr.empty()) {
		if (getHeader("transfer-encoding") == "chunked")
			return setError(HTTP_501_NOT_IMPLEMENTED);
		setParseState(PARSE_COMPLETE);
		return true;
	}
	std::size_t		   cl = 0;
	std::istringstream iss(clStr);
	if (!(iss >> cl))
		return setError(HTTP_400_BAD_REQUEST);

	size_t max = _srvConf->client_max_body_size;
	if (_locConf->has_max)
		max = _locConf->client_max_body_size;
	if (cl > max)
		return setError(HTTP_413_REQUEST_ENTITY_TOO_LARGE);
	if (cl == 0) {
		setParseState(PARSE_COMPLETE);
		return true;
	}
	if (buf.size() - _parsePos < cl)
		return false;
	_body = buf.substr(_parsePos, cl);
	_parsePos += cl;
	setParseState(PARSE_COMPLETE);
	return true;
}

bool Request::parse(const std::string& recvBuffer) {
	if (_state == PARSE_COMPLETE)
		return isComplete();
	if (_state == PARSE_REQUEST_LINE && !parseRequestLine(recvBuffer))
		return isComplete();
	if (_state == PARSE_HEADERS && !parseHeaders(recvBuffer))
		return isComplete();
	if (_state == PARSE_BODY)
		parseBody(recvBuffer);
	return isComplete();
}

bool Request::isValid() const {
	return _statusCode == HTTP_200_OK;
}

bool Request::isComplete() const {
	return _state == PARSE_COMPLETE;
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

std::string Request::getClientIp() const {
	return _clientIp;
}

std::string Request::getServerPort() const {
	return _serverPort;
}

std::string Request::getServerIp() const {
	return _serverIp;
}

std::string Request::getBody() const {
	return _body;
}

const Request::HeaderMap& Request::getHeaders() const {
	return _headers;
}

std::string Request::getHeader(const std::string& key) const {
	ConstHeaderIt it = _headers.find(toLower(key));
	return (it != _headers.end()) ? it->second : "";
}

const LocationConfig* Request::getLocationConf() const {
	return _locConf;
}

const ServerConfig* Request::getConf() const {
	return _srvConf;
}

bool Request::hasCgi() const {
	return _hasCgi;
}

std::string Request::resolveFullPath() const {
	std::string root = !_locConf->root.empty() ? _locConf->root : _srvConf->root;
	std::string rest = resolvePath();
	std::string loc	 = _locConf->path;

	if (_locConf->isAlias) {
		if (loc != "/" && rest.find(loc) == 0
			&& (rest.length() == loc.length() || rest[loc.length()] == '/')) {
			rest = rest.substr(loc.length());
			if (rest.empty()) {
				rest = "/";
			}
		}
	}

	if (_hasCgi) {
		for (std::map<std::string, std::string>::const_iterator it = _locConf->cgi.begin();
			 it != _locConf->cgi.end(); it++) {
			size_t pos = rest.find(it->first);
			if (pos != std::string::npos) {
				rest = rest.substr(0, pos + it->first.length());
				break;
			}
		}
	}
	if (root[root.length() - 1] == '/' && rest[0] == '/')
		return root + rest.substr(1);
	if (root[root.length() - 1] != '/' && rest[0] != '/')
		return root + "/" + rest;
	return root + rest;
}

std::string Request::resolvePath() const {
	if (_uri.empty())
		return "";
	std::string str	 = _uri;
	std::size_t qpos = str.find('?');
	if (qpos != std::string::npos)
		str = str.substr(0, qpos);

	std::string				 segment;
	std::vector<std::string> cleanPath;
	std::stringstream		 ss(str);

	while (std::getline(ss, segment, '/')) {
		if (segment.empty() || segment == ".")
			continue;
		if (segment == "..") {
			if (!cleanPath.empty())
				cleanPath.pop_back();
		} else {
			cleanPath.push_back(segment);
		}
	}

	std::string buffer;
	for (size_t i = 0; i < cleanPath.size(); i++) {
		buffer += "/";
		buffer += cleanPath[i];
	}
	if (cleanPath.empty() || _uri[_uri.size() - 1] == '/')
		buffer += "/";

	return buffer;
}

bool Request::setError(CodeStatus code) {
	setStatus(code);
	setParseState(PARSE_COMPLETE);
	return false;
}
void Request::setParseState(ParseState state) {
	_state = state;
}
