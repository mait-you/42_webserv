#include "../../includes/HttpStatus.hpp"

HttpStatus::HttpStatus() : _statusCode(HTTP_200_OK), _statusMessage("OK") {
}

HttpStatus::HttpStatus(codeStatus codeStatus, const std::string &message)
	: _statusCode(codeStatus), _statusMessage(message) {
}

HttpStatus::HttpStatus(const HttpStatus &other)
	: _statusCode(other._statusCode), _statusMessage(other._statusMessage) {
}

HttpStatus &HttpStatus::operator=(const HttpStatus &other) {
	if (this != &other) {
		_statusCode	   = other._statusCode;
		_statusMessage = other._statusMessage;
	}
	return *this;
}

HttpStatus::~HttpStatus() {
}

HttpStatus::codeStatus HttpStatus::getStatusCode() const {
	return _statusCode;
}

std::string HttpStatus::getStatusMessage() const {
	return _statusMessage;
}

void HttpStatus::setStatus(codeStatus codeStatus, const std::string &message) {
	_statusCode	   = codeStatus;
	_statusMessage = message;
}

bool HttpStatus::isError() const {
	return static_cast<int>(_statusCode) >= HTTP_400_BAD_REQUEST;
}

bool HttpStatus::isSuccess() const {
	int c = static_cast<int>(_statusCode);
	return c >= HTTP_200_OK && c < 300;
}

std::string HttpStatus::defaultMessage(codeStatus codeStatus) {
	switch (codeStatus) {
	case HTTP_200_OK:
		return "OK";
	case HTTP_201_CREATED:
		return "Created";
	case HTTP_202_ACCEPTED:
		return "Accepted";
	case HTTP_204_NO_CONTENT:
		return "No Content";
	case HTTP_301_MOVED_PERMANENTLY:
		return "Moved Permanently";
	case HTTP_302_FOUND:
		return "Found";
	case HTTP_304_NOT_MODIFIED:
		return "Not Modified";
	case HTTP_400_BAD_REQUEST:
		return "Bad Request";
	case HTTP_401_UNAUTHORIZED:
		return "Unauthorized";
	case HTTP_403_FORBIDDEN:
		return "Forbidden";
	case HTTP_404_NOT_FOUND:
		return "Not Found";
	case HTTP_500_INTERNAL_SERVER_ERROR:
		return "Internal Server Error";
	case HTTP_501_NOT_IMPLEMENTED:
		return "Not Implemented";
	case HTTP_502_BAD_GATEWAY:
		return "Bad Gateway";
	case HTTP_503_SERVICE_UNAVAILABLE:
		return "Service Unavailable";
	default:
		return "Unknown";
	}
}
