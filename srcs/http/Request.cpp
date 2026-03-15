#include "../../includes/Request.hpp"

Request::Request()
		: HttpStatus(HTTP_200_OK, "OK"), _srvConf(NULL), _locConf(NULL), _state(PARSE_REQUEST_LINE),
		  _parsePos(0), _requestComplete(false), _hasCgi(false) {}

Request::Request(const ServerConfig* serverConfig)
		: HttpStatus(HTTP_200_OK, "OK"), _srvConf(serverConfig), _locConf(NULL),
		  _state(PARSE_REQUEST_LINE), _parsePos(0), _requestComplete(false), _hasCgi(false) {}

Request::Request(const Request& other)
		: HttpStatus(other), _srvConf(other._srvConf), _locConf(other._locConf),
		  _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _state(other._state),
		  _parsePos(other._parsePos), _requestComplete(other._requestComplete),
		  _hasCgi(other._hasCgi) {}

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
		_state			 = other._state;
		_parsePos		 = other._parsePos;
		_requestComplete = other._requestComplete;
		_hasCgi			 = other._hasCgi;
	}
	return *this;
}

Request::~Request() {}

void Request::parseRequestLine(const std::string& buf) {
	std::string line;
	if (!getLine(buf, _parsePos, line))
		return;
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
	// 	setError(UNSUPPORTED_VERSION);
	matchedLocation();
	detectCgi();
	setState(PARSE_HEADERS);
}

void Request::parseHeaders(const std::string& buf) {
	while (true) {
		std::string line;
		if (!getLine(buf, _parsePos, line))
			return;
		if (line.empty()) {
			if (isValidHeaders())
				return setState(PARSE_BODY);
			return setError(HTTP_400_BAD_REQUEST);
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

void Request::parseBody(const std::string& buf) {
	std::string clStr = getHeader("content-length");
	if (clStr.empty())
		return setState(PARSE_COMPLETE);
	std::size_t		   cl = 0;
	std::istringstream iss(clStr);
	if (!(iss >> cl))
		return setError(HTTP_400_BAD_REQUEST);
	if (cl > _srvConf->client_max_body_size)
		return setError(HTTP_400_BAD_REQUEST);
	std::size_t _bodyExpected = static_cast<std::size_t>(cl);
	if (_bodyExpected == 0)
		return setState(PARSE_COMPLETE);
	if (buf.size() - _parsePos < _bodyExpected)
		return;
	_body = buf.substr(_parsePos, _bodyExpected);
	_parsePos += _bodyExpected;
	setState(PARSE_COMPLETE);
}

bool Request::parse(const std::string& buf) {
	if (_state == PARSE_COMPLETE)
		return true;
	if (_state == PARSE_REQUEST_LINE)
		parseRequestLine(buf);
	if (_state == PARSE_HEADERS)
		parseHeaders(buf);
	if (_state == PARSE_BODY)
		parseBody(buf);
	return _state == PARSE_COMPLETE;
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

const ServerConfig* Request::getServerConf() const {
	return _srvConf;
}

bool Request::hasCgi() const {
	return _hasCgi;
}

std::string Request::resolveFullPath() const {
	std::string root = !_locConf->root.empty() ? _locConf->root : _srvConf->root;
	// std::string rest = resolvePath().substr(_locConf->path.length());
	std::string rest = resolvePath();
	return root + rest;
}

std::string Request::resolvePath() const {
	if (uri.empty())
		return "";
	std::string str = uri;
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
	if (cleanPath.empty() || uri[uri.size() - 1] == '/')
		buffer += "/";

	return buffer;
}

void Request::setError(codeStatus codeStatus) {
	setStatus(codeStatus, HttpStatus::defaultMessage(codeStatus));
	std::stringstream ss;
	ss << codeStatus;
	throw std::runtime_error(ss.str());
}

void Request::setState(ParseState state) {
	_state = state;
}

std::ostream& operator<<(std::ostream& out, const Request& req) {
	const std::string none = "(empty)";
	out << "Method:  " << (req.getMethod().empty() ? none : req.getMethod()) << "\n";
	out << "URI:     " << (req.getUri().empty() ? none : req.getUri()) << "\n";
	out << "Version: " << (req.getVersion().empty() ? none : req.getVersion()) << "\n";
	out << "--- Headers ---\n";
	const Request::HeaderMap& hdrs = req.getHeaders();
	if (hdrs.empty())
		out << none << "\n";
	else
		for (Request::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
			out << it->first << ": " << it->second << "\n";
	out << "--- Body ---\n";
	const std::string& body = req.getBody();
	if (body.empty()) {
		out << "(empty)\n";
	} else {
		out << "[binary " << body.size() << " bytes] ";
		out << std::hex << std::setfill('0');
		for (size_t i = 0; i < body.size() && i < 32; ++i)
			out << std::setw(2) << (unsigned int) (unsigned char) body[i] << " ";
		if (body.size() > 32)
			out << "...";
		out << std::dec << "\n";
		out << "--- Type ---\n";
		out << "  " << (req.hasCgi() ? "Dynamic" : "Static") << "\n";
	}
	return out;
}
