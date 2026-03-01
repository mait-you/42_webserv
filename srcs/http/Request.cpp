#include "../../includes/Request.hpp"

Request::Request()
		: _error(OK), _state(PARSE_REQUEST_LINE), _parsePos(0), _bodyExpected(0),
		  _requestComplete(false) {}

Request::Request(const Request& other)
		: _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _error(other._error), _state(other._state),
		  _parsePos(other._parsePos), _bodyExpected(other._bodyExpected),
		  _requestComplete(other._requestComplete) {}

Request& Request::operator=(const Request& other) {
	if (this != &other) {
		_method			 = other._method;
		_uri			 = other._uri;
		_version		 = other._version;
		_headers		 = other._headers;
		_body			 = other._body;
		_error			 = other._error;
		_state			 = other._state;
		_parsePos		 = other._parsePos;
		_bodyExpected	 = other._bodyExpected;
		_requestComplete = other._requestComplete;
	}
	return *this;
}

Request::~Request() {}

static std::string toLower(const std::string& s) {
	std::string r = s;
	for (std::size_t i = 0; i < r.size(); ++i)
		r[i] = static_cast<char>(std::tolower(r[i]));
	return r;
}

static std::string trimStr(const std::string& s) {
	std::size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	std::size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

bool Request::getLine(const std::string& buf, std::size_t& pos, std::string& line) const {
	std::size_t end = buf.find("\r\n", pos);
	if (end == std::string::npos)
		return false;
	line = buf.substr(pos, end - pos);
	pos	 = end + 2;
	return true;
}
void Request::parseRequestLine(const std::string& buf) {
	std::string line;
	if (!getLine(buf, _parsePos, line))
		return;
	std::istringstream iss(line);
	if (!(iss >> _method >> _uri >> _version))
		setError(BAD_REQUEST);
	std::string extra;
	if (iss >> extra)
		setError(BAD_REQUEST);
	if (!isValidMethod(_method))
		setError(NOT_IMPLEMENTED);
	if (!isValidUri(_uri))
		setError(_uri.size() > MAX_URI_LENGTH ? URI_TOO_LONG : BAD_REQUEST);
	if (!isValidVersion(_version))
		setError(UNSUPPORTED_VERSION);
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
			setError(BAD_REQUEST);
		}
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			setError(BAD_REQUEST);
		std::string key	  = toLower(trimStr(line.substr(0, colon)));
		std::string value = trimStr(line.substr(colon + 1));
		if (key.empty())
			setError(BAD_REQUEST);
		if (key == "content-length" && _headers.count(key))
			setError(BAD_REQUEST);
		_headers[key] = value;
	}
}

void Request::parseBody(const std::string& buf) {
	if (getHeader("transfer-encoding") == "chunked") {
		std::size_t termPos = buf.find("0\r\n\r\n", _parsePos);
		if (termPos == std::string::npos)
			return;
		_body = decodeChunked(buf, _parsePos);
		return setState(PARSE_COMPLETE);
	}
	std::string clStr = getHeader("content-length");
	if (clStr.empty())
		return setState(PARSE_COMPLETE);
	long long		   cl = 0;
	std::istringstream iss(clStr);
	if (!(iss >> cl) || cl < 0)
		setError(BAD_REQUEST);

	_bodyExpected = static_cast<std::size_t>(cl);
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

std::string Request::decodeChunked(const std::string& buf, std::size_t& pos) const {
	std::string body;

	while (pos < buf.size()) {
		std::string line;
		std::size_t tmp = pos;
		if (!getLine(buf, tmp, line))
			break;
		if (line.empty())
			break;

		std::size_t semi = line.find(';');
		if (semi != std::string::npos)
			line = line.substr(0, semi);

		std::size_t		   chunkSize = 0;
		std::istringstream iss(line);
		if (!(iss >> std::hex >> chunkSize))
			break;
		if (chunkSize == 0) {
			pos = tmp;
			break;
		}

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

const Request::HeaderMap& Request::getHeaders() const {
	return _headers;
}

std::string Request::getHeader(const std::string& key) const {
	ConstHeaderIt it = _headers.find(toLower(key));
	return (it != _headers.end()) ? it->second : "";
}

void Request::setError(HttpError err) {
	_error = err;
	std::stringstream ss;
	ss << static_cast<int>(err);
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
	}
	return out;
}
