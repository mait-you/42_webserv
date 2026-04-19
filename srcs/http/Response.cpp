#include "../../includes/http/Response.hpp"

#include "../../includes/http/MimeTypes.hpp"

Response::Response()
		: HttpStatus(HTTP_200_OK, "OK"), _hasCgiRunning(false), _sessions(NULL),
		  _isComplete(false) {}

Response::Response(std::map<std::string, SessionInfo>* session)
		: HttpStatus(HTTP_200_OK, "OK"), _hasCgiRunning(false), _sessions(session),
		  _isComplete(false) {}

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

Response::~Response() {}

HttpStatus::codeStatus Response::getStatusCode() const {
	return _statusCode;
}
const std::string& Response::getStatusMessage() const {
	return _statusMessage;
}
const Response::HeaderMap& Response::getHeaders() const {
	return _headers;
}
const std::string& Response::getBody() const {
	return _body;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
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

void Response::applyCgiHeaders(const std::string& rawHeaders, codeStatus& outStatus,
							   std::string& outMsg) {
	std::istringstream stream(rawHeaders);
	std::string		   line;

	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		if (colon + 1 < line.size() && line[colon + 1] == ' ')
			line.erase(colon + 1, 1);

		std::string key = line.substr(0, colon);
		std::string val = (colon + 1 < line.size()) ? line.substr(colon + 1) : "";

		if (key == "Content-Length")
			continue;
		else if (key == "Status") {
			if (val.size() >= 4) {
				outStatus = static_cast<codeStatus>(std::atoi(val.c_str()));
				outMsg	  = val.substr(4);
			}
		} else
			setHeader(key, val);
	}
}
bool Response::processCgiOutput(const Request& request) {
	std::ifstream file(_runningCgi.resPath.c_str());
	if (!file.is_open()) {
		unlink(_runningCgi.resPath.c_str());
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return false;
	}

	std::ostringstream oss;
	oss << file.rdbuf();
	file.close();
	unlink(_runningCgi.resPath.c_str());

	std::string raw	   = oss.str();
	size_t		sepPos = raw.find("\r\n\r\n");
	size_t		skip   = 4;

	if (sepPos == std::string::npos) {
		sepPos = raw.find("\n\n");
		skip   = 2;
	}

	if (sepPos == std::string::npos) {
		setStatus(HTTP_200_OK, "OK");
		setBody(raw);
		return true;
	}

	codeStatus	cgiStatus = HTTP_200_OK;
	std::string cgiMsg	  = "OK";

	applyCgiHeaders(raw.substr(0, sepPos), cgiStatus, cgiMsg);
	setStatus(cgiStatus, cgiMsg);
	setBody(raw.substr(sepPos + skip));
	return true;
}

bool Response::pollCgi(const Request& request) {
	int	  wstatus;
	pid_t result = waitpid(_runningCgi.pid, &wstatus, WNOHANG);

	if (result == 0) {
		if (std::time(0) - _runningCgi.startTime <= 5)
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
	setStatus(static_cast<codeStatus>(loc->redirect_code));
	setHeader("Location", loc->redirect_url);
}

void Response::handleByMethod(const Request& request) {
	const std::string& m = request.getMethod();
	if (m == "GET")
		handleGet(request);
	else if (m == "POST")
		handlePost(request);
	else if (m == "DELETE")
		handleDelete(request);
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
	else
		handleByMethod(request);

	if (_hasCgiRunning)
		return "";
	return buildSendBuffer();
}

std::string Response::buildSendBuffer() {
	std::ostringstream oss;

	oss << HTTP_VERSION << " " << _statusCode << " " << _statusMessage << "\r\n";

	for (ConstHeaderIt it = _headers.begin(); it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";

	bool noBody = (_statusCode == HTTP_204_NO_CONTENT || _statusCode == HTTP_304_NOT_MODIFIED);

	if (!noBody)
		oss << "Content-Length: " << _body.size() << "\r\n";

	oss << "\r\n";

	if (!noBody)
		oss << _body;

	_isComplete = true;
	return oss.str();
}
