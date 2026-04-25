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

void Request::parseMultipart(const std::string& boundary) {
	/* RFC 7578 §4.1 — delimiter = "--" + boundary-value */
	const std::string delim		 = "--" + boundary;
	const std::string closeDelim = delim + "--";

	std::size_t pos = _body.find(delim);
	if (pos == std::string::npos)
		return; /* malformed — no delimiter found at all */

	/* Advance past the first delimiter line (includes trailing CRLF) */
	pos += delim.size();
	if (_body.compare(pos, 2, "\r\n") == 0)
		pos += 2;
	else
		return; /* malformed preamble */

	while (pos < _body.size()) {
		/* ── locate the next delimiter ──────────────────────────────────────
		 * RFC 7578 §4.1 — each part ends with CRLF before the next "--boundary"
		 * so we search for "\r\n--boundary" to find the exact end of this part */
		const std::string nextDelimPrefix = "\r\n" + delim;
		std::size_t		  partEnd		  = _body.find(nextDelimPrefix, pos);
		if (partEnd == std::string::npos)
			break; /* no closing delimiter — treat as end */

		std::string part = _body.substr(pos, partEnd - pos);

		/* ── split headers from body ────────────────────────────────────────
		 * RFC 7578 §4.2 — part headers end at the first blank line "\r\n\r\n" */
		const std::string headerBodySep = "\r\n\r\n";
		std::size_t		  sepPos		= part.find(headerBodySep);
		if (sepPos == std::string::npos) {
			/* RFC 7578 §4.2 — Content-Disposition is REQUIRED; skip bad part */
			pos = partEnd + nextDelimPrefix.size();
			if (_body.compare(pos, 2, "\r\n") == 0)
				pos += 2;
			continue;
		}

		std::string headerBlock = part.substr(0, sepPos);
		std::string partBody	= part.substr(sepPos + headerBodySep.size());

		/* ── parse the part headers ─────────────────────────────────────── */
		MultipartField field;
		field.data = partBody;

		if (!parsePartHeaders(headerBlock, field)) {
			/* RFC 7578 §4.2 — Content-Disposition with "name" is REQUIRED;
			   silently skip any part that fails this check */
			pos = partEnd + nextDelimPrefix.size();
			if (_body.compare(pos, 2, "\r\n") == 0)
				pos += 2;
			continue;
		}

		_multipartFields.push_back(field);

		/* ── advance past the delimiter that ended this part ────────────── */
		pos = partEnd + nextDelimPrefix.size();

		/* RFC 7578 §4.1 — close-delimiter has "--" suffix → we are done */
		if (_body.compare(pos, 2, "--") == 0)
			break;

		/* skip the CRLF after a normal delimiter before the next part */
		if (_body.compare(pos, 2, "\r\n") == 0)
			pos += 2;
	}
}

/* ────────────────────────────────────────────────────────────────────────────
 * parsePartHeaders
 *
 * Iterates over every header line in `headerBlock` (lines split by "\r\n").
 *
 * RFC 7578 §4.2 — MUST have:
 *   Content-Disposition: form-data; name="<fieldname>"
 *   optionally:          filename="<filename>"
 *
 * RFC 7578 §4.4 — MAY have:
 *   Content-Type: <media-type>   (default: text/plain)
 *
 * RFC 7578 §4.8 — all other Content-* headers MUST be ignored.
 *
 * Returns false if Content-Disposition is absent or "name" param is missing.
 * ─────────────────────────────────────────────────────────────────────────── */
bool Request::parsePartHeaders(const std::string& headerBlock, MultipartField& field) const {
	/* RFC 7578 §4.4 — default content-type when header is absent */
	field.contentType = "text/plain";

	bool hasContentDisposition = false;

	std::size_t lineStart = 0;
	while (lineStart <= headerBlock.size()) {
		std::size_t lineEnd = headerBlock.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = headerBlock.size();

		std::string line = headerBlock.substr(lineStart, lineEnd - lineStart);
		lineStart		 = lineEnd + 2;

		if (line.empty())
			continue;

		/* split "Header-Name: value" */
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string hName  = toLower(trimStr(line.substr(0, colon)));
		std::string hValue = trimStr(line.substr(colon + 1));

		/* ── RFC 7578 §4.2 — Content-Disposition ────────────────────────── */
		if (hName == "content-disposition") {
			/* value must start with "form-data" token */
			if (hValue.compare(0, 9, "form-data") != 0)
				continue;

			/* extract required "name" parameter */
			std::string nameVal = extractParam(hValue, "name");
			if (nameVal.empty())
				return false; /* RFC 7578 §4.2 — "name" is REQUIRED */

			field.name = nameVal;

			/* RFC 7578 §4.2 — "filename" is OPTIONAL */
			std::string filenameVal = extractParam(hValue, "filename");
			if (!filenameVal.empty())
				field.filename = filenameVal;

			hasContentDisposition = true;

			/* ── RFC 7578 §4.4 — Content-Type ───────────────────────────────── */
		} else if (hName == "content-type") {
			field.contentType = hValue;

			/* ── RFC 7578 §4.8 — all other Content-* headers are ignored ────── */
		}
	}

	return hasContentDisposition;
}

/* ────────────────────────────────────────────────────────────────────────────
 * extractParam
 *
 * Extracts the value of a named parameter from a header-field value string.
 *
 * Example input : "form-data; name=\"user\"; filename=\"a.txt\""
 * extractParam(..., "name")     → "user"
 * extractParam(..., "filename") → "a.txt"
 *
 * RFC 7578 §4.2 — parameter values are quoted-strings; we strip the quotes.
 * RFC 7578 §2   — percent-encoded filenames are returned as-is (caller decodes).
 * ─────────────────────────────────────────────────────────────────────────── */
std::string Request::extractParam(const std::string& headerValue, const std::string& param) const {
	/* search for "param=" (case-insensitive via toLower on the full value) */
	std::string lowerValue = toLower(headerValue);
	std::string token	   = param + "=";

	std::size_t pos = lowerValue.find(token);
	if (pos == std::string::npos)
		return "";

	pos += token.size(); /* move past "param=" */

	/* RFC 7578 §4.2 — values are typically quoted-strings */
	if (pos < headerValue.size() && headerValue[pos] == '"') {
		++pos; /* skip opening quote */
		std::size_t endQuote = headerValue.find('"', pos);
		if (endQuote == std::string::npos)
			return ""; /* malformed quoted-string */
		return headerValue.substr(pos, endQuote - pos);
	}

	/* unquoted token: ends at ';' or end-of-string */
	std::size_t endPos = headerValue.find(';', pos);
	if (endPos == std::string::npos)
		endPos = headerValue.size();

	return trimStr(headerValue.substr(pos, endPos - pos));
}

/* ─────────────────────────────────────────────────────────────────────────── */

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

			std::string ct = getHeader("Content-Type");
			_parseState	   = PARSE_DONE;

			/* RFC 7578 §4.1 — detect multipart/form-data and extract boundary */
			if (ct.find("multipart/form-data") != std::string::npos) {
				std::string bnd = extractParam(ct, "boundary");
				if (!bnd.empty())
					parseMultipart(bnd);
				else
					setError(HTTP_400_BAD_REQUEST); /* boundary is REQUIRED (§4.1) */
			}
		}
	}

	if (_parseState == PARSE_DONE)
		setStatus(HTTP_200_OK);
	return true;
}

void Request::processLine(const std::string& line) {
	if (_parseState == PARSE_REQUEST_LINE) {
		parseRequestLine(line);
	} else if (_parseState == PARSE_HEADERS) {
		if (line.empty()) {
			std::string cl = getHeader("content-length");
			if (!cl.empty()) {
				_contentLength = static_cast<std::size_t>(std::atol(cl.c_str()));
				if (_contentLength > _srvConf->client_max_body_size)
					return setError(HTTP_400_BAD_REQUEST);
				_parseState = (_contentLength > 0) ? PARSE_BODY : PARSE_DONE;
			} else {
				_contentLength = 0;
				_parseState	   = PARSE_DONE;
			}
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
	if (_httpVersion == HTTP_0_9)
		return setError(HTTP_400_BAD_REQUEST);

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

/* ── Getters ─────────────────────────────────────────────────────────────── */

bool Request::isComplete() const {
	return (_parseState == PARSE_DONE || _parseState == PARSE_ERROR);
}
bool Request::isValid() const {
	return isSuccess();
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
