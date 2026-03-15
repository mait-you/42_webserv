#include "../../includes/Response.hpp"
#include "../../includes/MimeTypes.hpp"

Response::Response()
	: HttpStatus(HTTP_200_OK, "OK"), _statusCode(HTTP_200_OK), _statusMessage("OK"),
	  _hasCgiRunning(false) {
}

Response::Response(const Response &other)
	: HttpStatus(other), _statusCode(other._statusCode), _statusMessage(other._statusMessage),
	_headers(other._headers), _body(other._body),
	_hasCgiRunning(other._hasCgiRunning), _runningCgi(other._runningCgi) {
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_statusCode		= other._statusCode;
		_statusMessage	= other._statusMessage;
		_headers		= other._headers;
		_body			= other._body;
		_hasCgiRunning	= other._hasCgiRunning;
		_runningCgi		= other._runningCgi;
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

std::string Response::build(Request &request) {

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
		// } else if (!allowedMethods(locConfig, request)) {
		// 	errorPage(srv, locConfig, 405);
		// } else if (!bodySize(srv, request)) {
		// 	errorPage(srv, locConfig, 413);
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
