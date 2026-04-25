#include "../../includes/http/Request.hpp"

#include "../../includes/utils/Utils.hpp"

Request::Request()
		: HttpStatus(), _srvConf(NULL), _locConf(NULL), _contentLength(0),
		  _parseState(PARSE_REQUEST_LINE), _hasCgi(false) {}

Request::Request(const Socket* servSocket, const std::string& clientIp)
		: HttpStatus(), _srvConf(servSocket->getConf()), _locConf(NULL),
		  _serverPort(servSocket->getPort()), _serverIp(servSocket->getIp()), _clientIp(clientIp),
		  _contentLength(0), _parseState(PARSE_REQUEST_LINE), _hasCgi(false) {}

Request::Request(const Request& other)
		: HttpStatus(other), _srvConf(other._srvConf), _locConf(other._locConf),
		  _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _serverPort(other._serverPort),
		  _serverIp(other._serverIp), _clientIp(other._clientIp),
		  _contentLength(other._contentLength), _parseState(other._parseState),
		  _hasCgi(other._hasCgi), _multipartFields(other._multipartFields) {}

Request& Request::operator=(const Request& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_srvConf		 = other._srvConf;
		_locConf		 = other._locConf;
		_method			 = other._method;
		_uri			 = other._uri;
		_version		 = other._version;
		_headers		 = other._headers;
		_body			 = other._body;
		_serverPort		 = other._serverPort;
		_serverIp		 = other._serverIp;
		_clientIp		 = other._clientIp;
		_contentLength	 = other._contentLength;
		_parseState		 = other._parseState;
		_hasCgi			 = other._hasCgi;
		_multipartFields = other._multipartFields;
	}
	return *this;
}

Request::~Request() {}

void Request::parseMultipartHeaderLine(const std::string& line, MultipartField& field) {
	std::size_t colon = line.find(':');
	if (colon == std::string::npos)
		return;

	std::string name  = toLower(trimStr(line.substr(0, colon)));
	std::string value = trimStr(line.substr(colon + 1));

	if (name == "content-disposition") {
		field.name	   = extractParam(value, "name");
		field.filename = extractParam(value, "filename");
	} else if (name == "content-type")
		field.contentType = value;
}

void Request::parsePart(std::string& part) {
	MultipartField field;
	field.contentType = "text/plain"; /* RFC 7578 §4.4 default */

	std::string line;
	do {
		std::size_t pos = part.find("\r\n");
		if (pos == std::string::npos)
			return setError(HTTP_400_BAD_REQUEST);
		line = part.substr(0, pos);
		part.erase(0, pos + 2);
		if (!line.empty())
			parseMultipartHeaderLine(line, field);
	} while (!line.empty());

	if (field.name.empty())
		return setError(HTTP_400_BAD_REQUEST);
	field.data = part;
	_multipartFields.push_back(field);
}

void Request::parseMultipart(const std::string& boundary) {
	const std::string delim		 = "--" + boundary + "\r\n";
	const std::string closeDelim = "--" + boundary + "--" + "\r\n";
	std::string body = _body;

	std::size_t pos = body.find(delim);
	if (pos == std::string::npos || pos != 0)
		return setError(HTTP_400_BAD_REQUEST);
	body.erase(0, delim.size());
	while (!body.empty()) {
		std::size_t pos = body.find(delim);
		std::size_t len = delim.size();
		if (pos == std::string::npos) {
			pos = body.find(closeDelim);
			len = closeDelim.size();
		}
		if (pos == std::string::npos)
			return setError(HTTP_400_BAD_REQUEST);
		std::string part = body.substr(0, pos);
		body.erase(0, pos + len);
		parsePart(part);
		if (_parseState == PARSE_ERROR)
			return;
	}
}

std::string Request::extractParam(const std::string& headerValue, const std::string& param) const {
	std::string lowerValue = toLower(headerValue);
	std::string token	   = param + "=";

	std::size_t pos = lowerValue.find(token);
	if (pos == std::string::npos)
		return "";

	pos += token.size();

	if (pos < headerValue.size() && headerValue[pos] == '"') {
		++pos;
		std::size_t endQuote = headerValue.find('"', pos);
		if (endQuote == std::string::npos)
			return "";
		return headerValue.substr(pos, endQuote - pos);
	}

	std::size_t endPos = headerValue.find(';', pos);
	if (endPos == std::string::npos)
		endPos = headerValue.size();

	return trimStr(headerValue.substr(pos, endPos - pos));
}

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
	// ! i need a to store in disk
	if (_parseState == PARSE_BODY) {
		if (_method != "POST")
			_parseState = PARSE_DONE;
		if (buffer.size() >= _contentLength) {
			_body = buffer.substr(0, _contentLength);
			buffer.erase(0, _contentLength);
			_parseState	   = PARSE_DONE;
			std::string ct = getHeader("Content-Type");
			if (ct.find("multipart/form-data") != std::string::npos) {
				std::string bnd = extractParam(ct, "boundary");
				if (!bnd.empty())
					parseMultipart(bnd);
				else
					setError(HTTP_400_BAD_REQUEST);
			}
		}
	}
	return true;
}

void Request::processLine(const std::string& line) {
	if (_parseState == PARSE_REQUEST_LINE) {
		parseRequestLine(line);
	} else if (_parseState == PARSE_HEADERS) {
		if (line.empty()) {
			std::string cl = getHeader("Content-length");
			if (!cl.empty()) {
				_contentLength = static_cast<std::size_t>(std::atol(cl.c_str()));
				if (_contentLength > _srvConf->client_max_body_size)
					return setError(HTTP_400_BAD_REQUEST);
				_parseState = (_contentLength > 0) ? PARSE_BODY : PARSE_DONE;

			} else if (_method == "POST") {
				return setError(HTTP_204_NO_CONTENT);
			} else {
				_contentLength = 0;
				_parseState	   = PARSE_DONE;
			}
		} else if (_httpVersion == HTTP_0_9) {
			return setError(HTTP_400_BAD_REQUEST);
		} else
			parseHeaderLine(line);
	}
}

void Request::parseRequestLine(const std::string& line) {
	std::size_t first = line.find(' ');
	if (first == std::string::npos)
		return setError(HTTP_400_BAD_REQUEST);
	_method = line.substr(0, first);

	std::size_t second = line.find(' ', first + 1);
	if (second == std::string::npos) {
		_uri = line.substr(first + 1);
	} else {
		_uri	 = line.substr(first + 1, second - first - 1);
		_version = line.substr(second + 1);
	}

	if (_method.empty() || _uri.empty())
		return setError(HTTP_400_BAD_REQUEST);

	if (!_version.empty() && _version.find(' ') != std::string::npos)
		return setError(HTTP_400_BAD_REQUEST);

	if (_version.empty() && _method == "GET")
		_httpVersion = HTTP_0_9;
	else if (_version == "HTTP/1.0")
		_httpVersion = HTTP_1_0;
	else if (_version == "HTTP/1.1")
		_httpVersion = HTTP_1_1;
	else
		return setError(HTTP_400_BAD_REQUEST);

	if (_method != "GET" && _method != "POST" && _method != "DELETE")
		return setError(HTTP_501_NOT_IMPLEMENTED);

	if (!isValidUri(_uri))
		return setError(HTTP_400_BAD_REQUEST);

	_resolveUri = resolvePath(_uri);
	if (!matchLocation())
		return setError(HTTP_400_BAD_REQUEST);
	_resolveFullUri = resolveFullPath();

	detectCgi();
	_parseState = PARSE_HEADERS;
}

void Request::parseHeaderLine(const std::string& line) {
	std::size_t colon = line.find(':');
	if (colon == std::string::npos)
		return setError(HTTP_400_BAD_REQUEST);

	std::string name  = line.substr(0, colon);
	std::string value = trimStr(line.substr(colon + 1));

	if (name.empty())
		return setError(HTTP_400_BAD_REQUEST);

	if (std::isspace((unsigned char) name[name.size() - 1]))
		return setError(HTTP_400_BAD_REQUEST);

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

	const std::string forbidden = "\"<>\\^~`{}|";

	for (std::size_t i = 0; i < uri.size(); ++i) {
		unsigned char c = uri[i];
		if (c < 33 || c == 127)
			return false;
		if (forbidden.find(c) != std::string::npos)
			return false;
	}
	return true;
}

bool Request::matchLocation() {
	if (!_srvConf)
		return false;

	const std::string& uri	   = _resolveUri;
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

	std::string path = _resolveFullUri;

	for (std::map<std::string, std::string>::const_iterator it = _locConf->cgi.begin();
		 it != _locConf->cgi.end(); ++it) {
		std::size_t pos = path.find(it->first);
		if (pos != std::string::npos) {
			path = path.substr(0, pos + it->first.size());
			break;
		}
	}

	std::string ext = getExtension(path);

	if (_locConf->cgi.count(ext) || _locConf->cgi.count("." + ext))
		_hasCgi = true;
}

void Request::setError(CodeStatus code) {
	setStatus(code);
	_parseState = PARSE_ERROR;
}

std::string Request::resolveFullPath() const {
	const std::string& root	   = !_locConf->root.empty() ? _locConf->root : _srvConf->root;
	std::string		   relPath = _resolveUri;
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
	return _parseState == PARSE_DONE;
}
bool Request::hasCgi() const {
	return _hasCgi;
}

const std::string& Request::getMethod() const {
	return _method;
}
const std::string& Request::getUri() const {
	return _uri;
}
const std::string& Request::getVersion() const {
	return _version;
}
const std::string& Request::getBody() const {
	return _body;
}
const std::string& Request::getClientIp() const {
	return _clientIp;
}
const std::string& Request::getServerPort() const {
	return _serverPort;
}
const std::string& Request::getServerIp() const {
	return _serverIp;
}

const Request::HeaderMap& Request::getHeaders() const {
	return _headers;
}
const std::string& Request::getResolvePath() const {
	return _resolveUri;
}
const std::string& Request::getResolveFullPath() const {
	return _resolveFullUri;
}
const LocationConfig* Request::getLocationConf() const {
	return _locConf;
}
const ServerConfig* Request::getConf() const {
	return _srvConf;
}
const Request::MultipartFields& Request::getMultipartFields() const {
	return _multipartFields;
}

std::string Request::getHeader(const std::string& key) const {
	ConstHeaderIt it = _headers.find(toLower(key));
	return (it != _headers.end()) ? it->second : "";
}
