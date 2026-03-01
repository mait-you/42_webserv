#include "../../includes/Request.hpp"

Request::Request()
		: HttpStatus(HTTP_200_OK, "OK"), _state(PARSE_REQUEST_LINE), _parsePos(0),
		  _bodyExpected(0), _requestComplete(false) {}

Request::Request(const Request& other)
		: HttpStatus(other), _method(other._method), _uri(other._uri), _version(other._version),
		  _headers(other._headers), _body(other._body), _state(other._state),
		  _parsePos(other._parsePos), _bodyExpected(other._bodyExpected),
		  _requestComplete(other._requestComplete) {}

Request& Request::operator=(const Request& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_method			 = other._method;
		_uri			 = other._uri;
		_version		 = other._version;
		_headers		 = other._headers;
		_body			 = other._body;
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

static bool getLine(const std::string& buf, std::size_t& pos, std::string& line) {
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
		setError(HTTP_400_BAD_REQUEST);
	std::string extra;
	if (iss >> extra)
		setError(HTTP_400_BAD_REQUEST);
	if (!isValidMethod(_method))
		setError(HTTP_501_NOT_IMPLEMENTED);
	if (!isValidUri(_uri))
		setError(HTTP_400_BAD_REQUEST);
	// if (!isValidVersion(_version))
	// 	setError(UNSUPPORTED_VERSION);
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
			setError(HTTP_400_BAD_REQUEST);
		}
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			setError(HTTP_400_BAD_REQUEST);
		std::string key	  = toLower(trimStr(line.substr(0, colon)));
		std::string value = trimStr(line.substr(colon + 1));
		if (key.empty())
			setError(HTTP_400_BAD_REQUEST);
		if (key == "content-length" && _headers.count(key))
			setError(HTTP_400_BAD_REQUEST);
		_headers[key] = value;
	}
}

void Request::parseBody(const std::string& buf) {
	std::string clStr = getHeader("content-length");
	if (clStr.empty())
		return setState(PARSE_COMPLETE);
	long long		   cl = 0;
	std::istringstream iss(clStr);
	if (!(iss >> cl) || cl < 0)
		setError(HTTP_400_BAD_REQUEST);

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

bool Request::isValid() const {
	return _statusCode == 200;
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

void Request::setError(Code code) {
	setStatus(code, HttpStatus::defaultMessage(code));
	std::stringstream ss;
	ss << code;
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
