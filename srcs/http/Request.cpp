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



/*
 * RFC 1945 §4   — HTTP/1.0 message format:
 *                 Request-Line
 *                 *( Header CRLF )
 *                 CRLF
 *                 [ message-body ]
 *
 * We read one CRLF-terminated line at a time from the caller's buffer.
 * The buffer shrinks as each line is consumed.
 * Returns true once the message is complete (done or error).
 */
bool Request::parse(std::string& buffer) {
	/* Phase 1 & 2: read lines until blank line closes the header section. */
	while (_parseState == PARSE_REQUEST_LINE || _parseState == PARSE_HEADERS) {
		/* RFC 1945 §2.2 — lines end with CRLF */
		std::size_t pos = buffer.find("\r\n");
		if (pos == std::string::npos)
			break; /* wait for more data */

		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2); /* consume line + CRLF */
		processLine(line);

		if (_parseState == PARSE_ERROR)
			return true;
	}

	/* Phase 3: collect body bytes up to Content-Length. */
	if (_parseState == PARSE_BODY) {
		if (buffer.size() >= _contentLength) {
			_body = buffer.substr(0, _contentLength);
			buffer.erase(0, _contentLength);
			_parseState = PARSE_DONE;
		}
		/* else: wait for more data */
	}

	return isComplete();
}

/*
 * Dispatch a single decoded line to the correct parser phase.
 * RFC 1945 §4 — blank line separates headers from body.
 */
void Request::processLine(const std::string& line) {
	if (_parseState == PARSE_REQUEST_LINE) {
		parseRequestLine(line);
		return;
	}

	if (_parseState == PARSE_HEADERS) {
		if (line.empty()) {
			/*
			 * RFC 1945 §4   — blank line ends header section.
			 * RFC 1945 §D.1 — Content-Length defines body size for POST.
			 *                  GET and HEAD have no body.
			 */
			std::string cl = getHeader("content-length");
			if (!cl.empty()) {
				_contentLength = static_cast<std::size_t>(std::atol(cl.c_str()));
				_parseState	   = (_contentLength > 0) ? PARSE_BODY : PARSE_DONE;
			} else {
				_contentLength = 0;
				_parseState	   = PARSE_DONE;
			}
		} else {
			parseHeaderLine(line);
		}
	}
}

/*
 * RFC 1945 §5.1 — Request-Line = Method SP Request-URI SP HTTP-Version CRLF
 *
 * Exactly three tokens separated by single SP.
 * Any extra token → 400.
 *
 * RFC 1945 §5.1.1 — Method tokens defined: GET, HEAD, POST (extension allowed).
 *                   We support GET, POST, DELETE; others → 501.
 *
 * RFC 1945 §3.1   — HTTP-Version = "HTTP" "/" 1*DIGIT "." 1*DIGIT
 *                   We accept HTTP/1.0 and tolerate HTTP/1.1.
 */
void Request::parseRequestLine(const std::string& line) {
	std::istringstream ss(line);
	std::string		   extra;

	if (!(ss >> _method >> _uri >> _version))
		return setError(HTTP_400_BAD_REQUEST);
	if (ss >> extra)
		return setError(HTTP_400_BAD_REQUEST); /* more than 3 tokens */

	/* RFC 1945 §5.1.1 — unsupported method */
	if (_method != "GET" && _method != "POST" && _method != "DELETE")
		return setError(HTTP_501_NOT_IMPLEMENTED);

	/* RFC 1945 §3.2 — validate URI characters */
	if (!isValidUri(_uri))
		return setError(HTTP_400_BAD_REQUEST);

	/* RFC 1945 §3.1 — only full HTTP/x.x requests accepted */
	if (_version != "HTTP/1.0" && _version != "HTTP/1.1")
		return setError(HTTP_400_BAD_REQUEST);

	/* Match URI to a configured location block. */
	if (!matchLocation())
		return setError(HTTP_404_NOT_FOUND);

	detectCgi();
	_parseState = PARSE_HEADERS;
}

/*
 * RFC 1945 §4.2 — Message-Header = field-name ":" [ field-value ]
 *
 * Rules enforced:
 *   - Colon must be present.
 *   - field-name must not be empty.
 *   - No whitespace between field-name and colon (RFC 1945 §4.2).
 *   - field-name characters: printable US-ASCII, no CTL (RFC 1945 §2.2).
 *   - field-name is stored lowercased for case-insensitive lookup.
 *   - field-value leading/trailing whitespace is trimmed (RFC 1945 §4.2).
 */
void Request::parseHeaderLine(const std::string& line) {
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


/*
 * RFC 1945 §3.2   — Request-URI = absoluteURI | abs_path | "*"
 * RFC 1945 §2.2   — CTL chars (0-31, 127) are forbidden.
 * RFC 1945 §3.2.2 — Only printable US-ASCII is safe inside a URI.
 *
 * We also reject a small set of chars that are structurally unsafe
 * in a filesystem path or HTTP context even if not CTL.
 */
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



/*
 * Walk every configured location and pick the one whose path is the
 * longest prefix of the request URI.  Longest-match wins (like nginx).
 *
 * RFC 1945 §5.1.2 — The server maps the Request-URI to a local resource.
 *                   The mapping strategy is implementation-defined.
 */
bool Request::matchLocation() {
	if (!_srvConf)
		return false;

	const std::string& uri	   = resolvePath();
	std::size_t		   bestLen = 0;

	for (std::size_t i = 0; i < _srvConf->locations.size(); ++i) {
		const std::string& locPath = _srvConf->locations[i].path;

		/* The URI must start with the location path... */
		if (uri.compare(0, locPath.size(), locPath) != 0)
			continue;

		/* ...and the match must be at a path boundary (or exact). */
		if (locPath != "/" && uri.size() != locPath.size() && uri[locPath.size()] != '/')
			continue;

		if (locPath.size() > bestLen) {
			bestLen	 = locPath.size();
			_locConf = &_srvConf->locations[i];
		}
	}

	return (_locConf != NULL);
}


/*
 * RFC 1945 §11 — CGI is an extension; it is not defined in the spec.
 * We detect CGI by checking whether the resolved filesystem path has
 * a file extension that is registered in the location's CGI map.
 *
 * The extension check accepts both ".pl" and "pl" forms.
 */
void Request::detectCgi() {
	if (!_locConf || !_locConf->has_cgi)
		return;

	std::string path = resolveFullPath();

	/* Trim path to the CGI script (stop at the first matching extension). */
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
 * Strips the query string and resolves ".." segments.
 *
 * RFC 1945 §3.2 — The query component follows "?".
 * RFC 1945 §3.2 — Path segments "." and ".." follow normal URI rules.
 *
 * Example: "/a/../b?x=1" → "/b"
 */
std::string Request::resolvePath() const {
	if (_uri.empty())
		return "/";

	/* Strip query string */
	std::string path = _uri;
	std::size_t q	 = path.find('?');
	if (q != std::string::npos)
		path = path.substr(0, q);

	/* Resolve . and .. segments */
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

	/* Preserve trailing slash if the original URI had one. */
	if (clean.empty() || _uri[_uri.size() - 1] == '/')
		clean += "/";

	return clean;
}

/*
 * Builds the full filesystem path.
 *
 * RFC 1945 §5.1.2 — Server translates Request-URI to a resource.
 *                   Root and alias directives are our implementation.
 *
 * - Normal:  root + resolvePath()
 * - Alias:   root + resolvePath() with location prefix stripped
 *
 * Example (normal): root="/var/www", URI="/img/a.png" → "/var/www/img/a.png"
 * Example (alias):  root="/data",   loc="/img", URI="/img/a.png" → "/data/a.png"
 */
std::string Request::resolveFullPath() const {
	const std::string& root	   = !_locConf->root.empty() ? _locConf->root : _srvConf->root;
	std::string		   relPath = resolvePath();
	const std::string& locPath = _locConf->path;

	/* Alias: strip the location prefix from the URI. */
	if (_locConf->isAlias) {
		if (locPath != "/" && relPath.compare(0, locPath.size(), locPath) == 0
			&& (relPath.size() == locPath.size() || relPath[locPath.size()] == '/')) {
			relPath = relPath.substr(locPath.size());
			if (relPath.empty())
				relPath = "/";
		}
	}

	/* For CGI, trim the path to stop at the script extension. */
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

	/* Join root and relative path, avoiding double or missing slash. */
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

/*
 * RFC 1945 §5 — A request is valid when it parsed without error.
 * We use HTTP 200 as the "no error yet" sentinel in HttpStatus.
 */
bool Request::isValid() const {
	return (_statusCode == HTTP_200_OK);
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

/*
 * RFC 1945 §4.2 — Header field names are case-insensitive.
 * We store all names in lowercase, so lookup is always lowercase too.
 */
std::string Request::getHeader(const std::string& key) const {
	ConstHeaderIt it = _headers.find(toLower(key));
	return (it != _headers.end()) ? it->second : "";
}
