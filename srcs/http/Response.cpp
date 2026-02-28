#include "../../includes/Response.hpp"

#include "../../includes/MimeTypes.hpp"

Response::Response() : _statusCode(200), _statusMessage("OK") {}

Response::Response(const Response& other)
		: _statusCode(other._statusCode), _statusMessage(other._statusMessage),
		  _headers(other._headers), _body(other._body) {}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		_statusCode	   = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers	   = other._headers;
		_body		   = other._body;
	}
	return *this;
}

Response::~Response() {}

void Response::setStatus(int code, const std::string& message) {
	_statusCode	   = code;
	_statusMessage = message;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
}

std::string Response::build(Request& request, const std::vector<ServerConfig>& servers) {
	ServerConfig	srv		  = matchedServer(request, servers);
	LocationConfig* locConfig = matchedLocation(srv, request);

	if (!request.isValid()) {
		Request::HttpError error = request.getError();
		if (error == 400)
			errorPage(srv, locConfig, 400, "Bad Request");
		else if (error == 505)
			errorPage(srv, locConfig, 505, "HTTP Version Not Supported");
	} else if (!locConfig) {
		errorPage(srv, locConfig, 404, "Not Found");
	} else if (locConfig->has_redirect) {
		if (locConfig->redirect_code == 301)
			errorPage(srv, locConfig, 301, "Moved Permanently");
		else
			errorPage(srv, locConfig, 302, "Found");
		setHeader("Location", locConfig->redirect_url);
	} else if (!allowedMethods(locConfig, request)) {
		errorPage(srv, locConfig, 405, "Method Not Allowed");
	} else if (!bodySize(srv, request)) {
		errorPage(srv, locConfig, 413, "Payload Too Large");
	} else if (request.getMethod() == "GET") {
		handleGet(request, srv, locConfig);
	} else if (request.getMethod() == "POST") {
		handlePost(request, srv, locConfig);
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
