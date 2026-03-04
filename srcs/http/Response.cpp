#include "../../includes/Response.hpp"

#include "../../includes/MimeTypes.hpp"

Response::Response()
	: HttpStatus(HTTP_200_OK, "OK"), _statusCode(HTTP_200_OK), _statusMessage("OK") {
}

Response::Response(const Response &other)
	: HttpStatus(other), _statusCode(other._statusCode), _statusMessage(other._statusMessage),
	  _headers(other._headers), _body(other._body) {
}

Response &Response::operator=(const Response &other) {
	if (this != &other) {
		HttpStatus::operator=(other);
		_statusCode	   = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers	   = other._headers;
		_body		   = other._body;
	}
	return *this;
}

Response::~Response() {
}

void Response::setStatus(codeStatus codeStatus, const std::string &message) {
	_statusCode	   = codeStatus;
	_statusMessage = message;
}

void Response::setStatus(codeStatus codeStatus) {
	_statusCode	   = codeStatus;
	_statusMessage = defaultMessage(codeStatus);
}

void Response::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void Response::setBody(const std::string &body) {
	_body = body;
}

std::string Response::build(Request &request, const std::vector<ServerConfig> &servers) {
	ServerConfig	srv		  = matchedServer(request, servers);
	LocationConfig *locConfig = matchedLocation(srv, request);

	if (!request.isValid()) {
		codeStatus error = request.getStatusCode();
		if (error == HTTP_400_BAD_REQUEST)
			errorPage(srv, locConfig, HTTP_400_BAD_REQUEST);
		// else if (error == 505)
		// 	errorPage(srv, locConfig, 505);
	} else if (!locConfig) {
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
	} else if (locConfig->has_redirect) {
		if (locConfig->redirect_code == HTTP_301_MOVED_PERMANENTLY)
			setStatus(HTTP_301_MOVED_PERMANENTLY);
		else
			setStatus(HTTP_302_FOUND, "Found");
		setHeader("Location", locConfig->redirect_url);
	// } else if (!allowedMethods(locConfig, request)) {
	// 	errorPage(srv, locConfig, 405);
	// } else if (!bodySize(srv, request)) {
	// 	errorPage(srv, locConfig, 413);
	} else if (request.getMethod() == "GET") {
		handleGet(request, srv, locConfig);
	} else if (request.getMethod() == "POST") {
		handlePost(request, srv, locConfig);
	} else if (request.getMethod() == "DELETE") {
		handleDelete(request, srv, locConfig);
	}

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
