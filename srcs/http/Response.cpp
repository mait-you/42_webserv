#include "../../includes/Response.hpp"

#include "../../includes/MimeTypes.hpp"

Response::Response()
		: HttpStatus(HTTP_200_OK, "OK"), _statusCode(HTTP_200_OK), _statusMessage("OK"),
		  _hasCgiRunning(false), _responseReady(false) {}

Response::Response(const Response& other)
		: HttpStatus(other), _statusCode(other._statusCode), _statusMessage(other._statusMessage),
		  _headers(other._headers), _body(other._body), _hasCgiRunning(other._hasCgiRunning),
		  _runningCgi(other._runningCgi), _responseReady(other._responseReady) {}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_statusCode	   = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers	   = other._headers;
		_body		   = other._body;
		_hasCgiRunning = other._hasCgiRunning;
		_runningCgi	   = other._runningCgi;
		_responseReady = other._responseReady;
	}
	return *this;
}

Response::~Response() {}

void Response::setStatus(codeStatus codeStatus, const std::string& message) {
	_statusCode	   = codeStatus;
	_statusMessage = message;
}

void Response::setStatus(codeStatus codeStatus) {
	_statusCode	   = codeStatus;
	_statusMessage = defaultMessage(codeStatus);
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
void Response::_parseCgiHeaders(const std::string& headers, codeStatus& status) {
	std::istringstream stream(headers);
	std::string		   line;

	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		std::string val = line.substr(colon + 2);

		if (key == "Content-Length")
			continue;
		else if (key == "Status")
			status = static_cast<codeStatus>(std::atoi(val.c_str()));
		else
			setHeader(key, val);

		setHeader(key, val);
	}
}

bool Response::checkCgi(const Request& request) {
	if (!_hasCgiRunning)
		return false;

	int	  status;
	pid_t result = waitpid(_runningCgi.pid, &status, WNOHANG);
	if (result == 0)
		return false;

	_hasCgiRunning = false;

	if (!_runningCgi.bodyPath.empty())
		unlink(_runningCgi.bodyPath.c_str());

	// CGI failed
	if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		if (!_runningCgi.resPath.empty())
			unlink(_runningCgi.resPath.c_str());
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return _responseReady = true;
	}

	// Read output file
	std::ifstream file(_runningCgi.resPath.c_str());
	if (!file.is_open()) {
		unlink(_runningCgi.resPath.c_str());
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return _responseReady = true;
	}
	std::ostringstream oss;
	oss << file.rdbuf();
	file.close();
	unlink(_runningCgi.resPath.c_str());

	std::string raw = oss.str();

	std::cout << "wiiiiiiiiii3\n" << raw << "\nwiiiii3\n";

	// Split headers and body
	size_t sepPos = raw.find("\r\n\r\n");
	size_t skip	  = 4;
	if (sepPos == std::string::npos) {
		sepPos = raw.find("\n\n");
		skip   = 2;
	}

	if (sepPos == std::string::npos) {
		setStatus(HTTP_200_OK, "OK");
		setBody(raw);
		return _responseReady = true;
	}

	codeStatus cgiStatus = HTTP_200_OK;
	_parseCgiHeaders(raw.substr(0, sepPos), cgiStatus);
	setStatus(cgiStatus, "OK");
	setBody(raw.substr(sepPos + skip));
	return _responseReady = true;
}

std::string Response::build(Request& request) {
	if (_responseReady)
		return buildSendBuffer();
	if (!request.isValid()) {
		codeStatus error = request.getStatusCode();
		if (error == HTTP_400_BAD_REQUEST)
			errorPage(request, HTTP_400_BAD_REQUEST);
	} else if (!request.getLocationConf()) {
		errorPage(request, HTTP_404_NOT_FOUND);
	} else if (request.getLocationConf()->has_redirect) {
		if (request.getLocationConf()->redirect_code == HTTP_301_MOVED_PERMANENTLY)
			setStatus(HTTP_301_MOVED_PERMANENTLY);
		else
			setStatus(HTTP_302_FOUND);
		setHeader("Location", request.getLocationConf()->redirect_url);
	} else if (request.getMethod() == "GET") {
		handleGet(request);
	} else if (request.getMethod() == "POST") {
		handlePost(request);
	} else if (request.getMethod() == "DELETE") {
		handleDelete(request);
	}
	if (_hasCgiRunning)
		return "";
	return buildSendBuffer();
}

std::string Response::buildSendBuffer() const {
	std::ostringstream oss;

	oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
	for (ConstHeaderIt it = _headers.begin(); it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	oss << "Content-Length: " << _body.size() << "\r\n";
	oss << "\r\n";
	oss << _body;
	return oss.str();
}
