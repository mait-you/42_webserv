#include "../../includes/http/Response.hpp"

#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/utils/Utils.hpp"

Response::Response() : HttpStatus(), _hasCgiRunning(false), _sessions(NULL), _isComplete(false) {}

Response::Response(std::map<std::string, std::string>* session)
		: HttpStatus(), _hasCgiRunning(false), _sessions(session), _isComplete(false) {}

Response::Response(const Response& other)
		: HttpStatus(other), _headers(other._headers), _body(other._body),
		  _hasCgiRunning(other._hasCgiRunning), _runningCgi(other._runningCgi),
		  _sessions(other._sessions), _isComplete(other._isComplete) {}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_headers	   = other._headers;
		_body		   = other._body;
		_hasCgiRunning = other._hasCgiRunning;
		_runningCgi	   = other._runningCgi;
		_sessions	   = other._sessions;
		_isComplete	   = other._isComplete;
	}
	return *this;
}

void Response::operator=(const Request& req) {
	HttpStatus::operator=(req);
	if (_httpVersion != HTTP_0_9)
		_httpVersion = HTTP_1_0;
	if (_httpVersion == HTTP_0_9)
		_statusCode = HTTP_000_NO_CODE_STATUS;
}

Response::~Response() {}

HttpStatus::CodeStatus Response::getStatusCode() const {
	return _statusCode;
}

const Response::HeaderMap& Response::getHeaders() const {
	return _headers;
}
const std::string& Response::getBody() const {
	return _body;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	if (_httpVersion != HTTP_0_9)
		_headers.insert(std::make_pair(key, value));
}

void Response::setBody(const std::string& body) {
	_body = body;
}

bool Response::hasCgiRunning() const {
	return _hasCgiRunning;
}

bool Response::isComplete() const {
	return _isComplete;
}

void Response::applyCgiHeaders(const std::string& rawHeaders, CodeStatus& outStatus,
							   std::string& outLocation, bool& outHasContentType) {
	std::istringstream stream(rawHeaders);
	std::string		   line;

	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		std::string key = line.substr(0, colon);
		// RFC 3875 §6.3 — Whitespace is allowed in val
		std::string val = trimStr(line.substr(colon + 1));
		// RFC 3875 §6.1 — ignore script value
		if (key == "Content-Length")
			continue;
		// RFC 3875 §6.3.3 — Status is CGI-only; not forwarded as HTTP header
		if (key == "Status") {
			if (val.size() >= 3)
				outStatus = static_cast<CodeStatus>(std::atoi(val.c_str()));
			continue;
		}
		if (key == "Location")
			outLocation = val;
		if (key == "Content-Type")
			outHasContentType = true;
		setHeader(key, val);
	}
}

void Response::processCgiOutput(const Request& request) {
	std::ifstream file(_runningCgi.resPath.c_str(), std::ios::binary);
	if (!file) {
		unlink(_runningCgi.resPath.c_str());
		return errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
	}

	std::ostringstream oss;
	oss << file.rdbuf();
	file.close();
	unlink(_runningCgi.resPath.c_str());
	std::string raw = oss.str();
	// RFC 3875 §6.1 A script MUST always provide a non-empty response
	if (raw.empty())
		return errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);

	std::size_t pos = raw.find("\r\n\r\n");
	// RFC 3875 §6 — header block separated from body is required; no separator = malformed
	if (pos == std::string::npos)
		return errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);

	CodeStatus	cgiStatus = HTTP_200_OK;
	std::string location;
	bool		hasContentType = false;
	applyCgiHeaders(raw.substr(0, pos), cgiStatus, location, hasContentType);
	std::string body = raw.substr(pos + 4);
	// §6.2.2 — Local redirect
	if (!location.empty() && location[0] == '/') {
		if (!body.empty() || _headers.size() > 1)
			return setStatus(HTTP_500_INTERNAL_SERVER_ERROR);
		setStatus(HTTP_302_FOUND);
		return;
	}

	// §6.2.3 — Client redirect, no body
	if (!location.empty() && body.empty()) {
		setStatus(HTTP_302_FOUND);
		return;
	}

	// §6.2.4 — Client redirect with body
	if (!location.empty() && !body.empty() && hasContentType) {
		if (cgiStatus != HTTP_301_MOVED_PERMANENTLY && cgiStatus != HTTP_302_FOUND
			&& cgiStatus != HTTP_304_NOT_MODIFIED)
			return setStatus(HTTP_500_INTERNAL_SERVER_ERROR);
		setStatus(cgiStatus);
		setBody(body);
		return;
	}

	// §6.2.1 — Normal document response
	if (!hasContentType)
		return errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
	setStatus(cgiStatus);
	setBody(body);
}

bool Response::pollCgi(const Request& request) {
	int	  wstatus;
	pid_t result = waitpid(_runningCgi.pid, &wstatus, WNOHANG);

	if (result == 0) {
		// RFC 3875 §3.4 — server may terminate the script at any time
		if (std::time(0) - _runningCgi.startTime <= CLIENT_IDLE_TIMEOUT)
			return false;
		kill(_runningCgi.pid, SIGKILL);
		waitpid(_runningCgi.pid, NULL, 0);
		_hasCgiRunning = false;
		if (!_runningCgi.bodyPath.empty())
			unlink(_runningCgi.bodyPath.c_str());
		if (!_runningCgi.resPath.empty())
			unlink(_runningCgi.resPath.c_str());
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return true;
	}
	_hasCgiRunning = false;
	if (!_runningCgi.bodyPath.empty())
		unlink(_runningCgi.bodyPath.c_str());
	if (result == -1 || !WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
		if (!_runningCgi.resPath.empty())
			unlink(_runningCgi.resPath.c_str());
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return true;
	}
	processCgiOutput(request);
	return true;
}

void Response::handleRedirect(const Request& request) {
	const LocationConfig* loc = request.getLocationConf();
	setStatus(static_cast<CodeStatus>(loc->redirect_code));
	setHeader("Location", loc->redirect_url);
}

void Response::handleByMethod(Request& request) {
	const std::string& m = request.getMethod();
	if (m == "GET")
		handleGet(request);
	else if (m == "POST")
		handlePost(request);
	else if (m == "DELETE")
		handleDelete(request);
}

void Response::handleSession(const Request& request) {
	const Request::FormData& data = request.getFormData();

	Request::FormData::const_iterator it	 = data.find("theme");
	std::string						  cookie = request.getHeader("Cookie");
	if (it == data.end()) {
		if (cookie.empty()) {
			std::string id	 = randomSessionId();
			(*_sessions)[id] = "light";
			setHeader("Set-Cookie", "session_id=" + id + "; Path=/; HttpOnly;");
			setHeader("Set-Cookie", "theme=light; Path=/;");
			return;
		}
	}

	std::string		  theme = it->second.empty() ? "light" : it->second[0];
	std::stringstream ss(cookie);
	std::string		  line, sessionId;

	while (std::getline(ss, line, ';')) {
		size_t pos = line.find("session_id=");
		if (pos != std::string::npos)
			sessionId = line.substr(pos + 11);
	}
	if (it != data.end()) {
		if (!sessionId.empty()) {
			std::map<std::string, std::string>::iterator it;
			for (it = _sessions->begin(); it != _sessions->end(); it++) {
				if (it->first == sessionId) {
					it->second = theme;
					setHeader("Set-Cookie", "theme=" + theme + "; Path=/;");
					return;
				}
			}
		}

		std::string id	 = randomSessionId();
		(*_sessions)[id] = theme;
		setHeader("Set-Cookie", "session_id=" + id + "; Path=/; HttpOnly;");
		setHeader("Set-Cookie", "theme=" + theme + "; Path=/;");
	}
}

std::string Response::build(Request& request) {
	if (!request.isValid())
		errorPage(request, request.getStatusCode());
	else if (!request.getLocationConf())
		errorPage(request, HTTP_404_NOT_FOUND);
	else if (request.getLocationConf()->has_redirect)
		handleRedirect(request);
	else if (!allowedMethods(request))
		errorPage(request, HTTP_405_METHOD_NOT_ALLOWED);
	else {
		handleSession(request);
		handleByMethod(request);
	}

	if (_hasCgiRunning)
		return "";
	return buildSendBuffer();
}

std::string Response::buildSendBuffer() {
	std::ostringstream oss;

	if (_httpVersion == HTTP_0_9) {
		_isComplete = true;
		return _body;
	}
	bool noBody =
		(_statusCode == HTTP_204_NO_CONTENT || _statusCode == HTTP_304_NOT_MODIFIED) ? true : false;

	oss << getHttpVersion() << " " << _statusCode << " " << getStatusMessage() << "\r\n";

	for (ConstHeaderIt it = _headers.begin(); it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	if (!noBody)
		oss << "Content-Length: " << _body.size() << "\r\n";

	oss << "\r\n";

	if (!noBody)
		oss << _body;

	_isComplete = true;
	return oss.str();
}
