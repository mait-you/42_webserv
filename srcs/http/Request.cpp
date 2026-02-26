#include "../../includes/Request.hpp"

Request::Request()
	: _error(OK), _state(PARSE_REQUEST_LINE), _parsePos(0), _bodyExpected(0) {
}

Request::Request(const Request &other)
	: _method(other._method), _uri(other._uri), _version(other._version),
	  _headers(other._headers), _body(other._body), _error(other._error),
	  _state(other._state), _parsePos(other._parsePos),
	  _bodyExpected(other._bodyExpected) {
}

Request &Request::operator=(const Request &other) {
	if (this != &other) {
		_method		  = other._method;
		_uri		  = other._uri;
		_version	  = other._version;
		_headers	  = other._headers;
		_body		  = other._body;
		_error		  = other._error;
		_state		  = other._state;
		_parsePos	  = other._parsePos;
		_bodyExpected = other._bodyExpected;
	}
	return *this;
}

Request::~Request() {
}

bool Request::getLine(const std::string &buf, std::size_t &pos,
					  std::string &line) const {
	std::size_t end = buf.find("\r\n", pos);
	if (end == std::string::npos)
		return false;
	line = buf.substr(pos, end - pos);
	pos	 = end + 2;
	return true;
}

bool Request::parseRequestLine(const std::string &buf) {
	std::string line;
	if (!getLine(buf, _parsePos, line))
		return false;
	std::istringstream iss(line);
	if (!(iss >> _method >> _uri >> _version)) {
		_error = BAD_REQUEST;
		return false;
	}
	std::string extra;
	if (iss >> extra) {
		_error = BAD_REQUEST;
		return false;
	}
	if (!isValidMethod(_method)) {
		_error = NOT_IMPLEMENTED;
		return false;
	}
	if (!isValidUri(_uri)) {
		_error = (_uri.size() > MAX_URI_LENGTH) ? URI_TOO_LONG : BAD_REQUEST;
		return false;
	}
	if (!isValidVersion(_version)) {
		_error = UNSUPPORTED_VERSION;
		return false;
	}
	_state = PARSE_HEADERS;
	return true;
}

bool Request::parseHeaders(const std::string &buf) {
	while (true) {
		std::string line;
		if (!getLine(buf, _parsePos, line))
			return false;
		if (line.empty()) {
			_state = PARSE_BODY;
			return true;
		}
		std::size_t colon = line.find(':');
		if (colon == std::string::npos) {
			_error = BAD_REQUEST;
			return false;
		}
		std::string key	  = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));
		if (key.empty()) {
			_error = BAD_REQUEST;
			return false;
		}
		_headers[key] = value;
	}
}

bool Request::parseBody(const std::string &buf) {
	if (getHeader("Transfer-Encoding") == "chunked") {
		if (buf.find("0\r\n\r\n", _parsePos) == std::string::npos)
			return false;
		_body  = decodeChunked(buf, _parsePos);
		_state = PARSE_COMPLETE;
		return true;
	}
	std::string clStr = getHeader("Content-Length");
	if (clStr.empty()) {
		_state = PARSE_COMPLETE;
		return true;
	}
	std::istringstream iss(clStr);
	if (!(iss >> _bodyExpected) || _bodyExpected == 0) {
		_error = BAD_REQUEST;
		return false;
	}
	if (buf.size() - _parsePos < _bodyExpected)
		return false;
	_body = buf.substr(_parsePos, _bodyExpected);
	_parsePos += _bodyExpected;
	_state = PARSE_COMPLETE;
	return true;
}

bool Request::parse(const std::string &buf) {
	if (_error != OK)
		return false;
	if (_state == PARSE_REQUEST_LINE && !parseRequestLine(buf))
		return false;
	if (_error != OK)
		return false;
	if (_state == PARSE_HEADERS && !parseHeaders(buf))
		return false;
	if (_error != OK)
		return false;
	if (_state == PARSE_BODY && !parseBody(buf))
		return false;
	return _state == PARSE_COMPLETE;
}

std::string Request::decodeChunked(const std::string &buf,
								   std::size_t		  pos) const {
	std::string body;
	while (pos < buf.size()) {
		std::string line;
		std::size_t tmp = pos;
		if (!getLine(buf, tmp, line))
			break;
		if (line.empty())
			break;

		std::size_t		   chunkSize = 0;
		std::istringstream iss(line);
		if (!(iss >> std::hex >> chunkSize) || chunkSize == 0)
			break;

		pos = tmp;
		if (pos + chunkSize > buf.size())
			break;

		body += buf.substr(pos, chunkSize);
		pos += chunkSize;

		if (pos + 2 <= buf.size())
			pos += 2;
	}
	return body;
}

Request::HttpError Request::getError() const {
	return _error;
}
bool Request::isValid() const {
	return _error == OK;
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

const Request::HeaderMap &Request::getHeaders() const {
	return _headers;
}

std::string Request::getHeader(const std::string &key) const {
	ConstHeaderIt it = _headers.find(key);
	return (it != _headers.end()) ? it->second : "";
}

std::ostream &operator<<(std::ostream &out, const Request &req) {
	out << req.getMethod() << " " << req.getUri() << " " << req.getVersion()
		<< "\n";
	const Request::HeaderMap &hdrs = req.getHeaders();
	for (Request::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		out << it->first << ": " << it->second << "\n";
	if (!req.getBody().empty())
		out << "\n" << req.getBody() << "\n";
	return out;
}
