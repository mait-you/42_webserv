#include "../../includes/http/HttpStatus.hpp"

HttpStatus::HttpStatus() : _statusCode(HTTP_200_OK) {}

HttpStatus::HttpStatus(CodeStatus statusCode) : _statusCode(statusCode) {}

HttpStatus::HttpStatus(const HttpStatus& other) : _statusCode(other._statusCode) {}

HttpStatus& HttpStatus::operator=(const HttpStatus& other) {
	if (this != &other)
		_statusCode = other._statusCode;
	return *this;
}

HttpStatus::~HttpStatus() {}

HttpStatus::CodeStatus HttpStatus::getStatusCode() const {
	return _statusCode;
}

std::string HttpStatus::getStatusMessage() const {
	return defaultMessage();
}

void HttpStatus::setStatus(CodeStatus CodeStatus) {
	_statusCode = CodeStatus;
}

bool HttpStatus::isError() const {
	return _statusCode >= HTTP_400_BAD_REQUEST;
}

bool HttpStatus::isSuccess() const {
	return _statusCode >= HTTP_200_OK && _statusCode <= HTTP_204_NO_CONTENT;
}

std::string HttpStatus::defaultMessage() const {
	switch (_statusCode) {
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
		case HTTP_405_METHOD_NOT_ALLOWED:
			return "Method Not Allowed";
		case HTTP_413_REQUEST_ENTITY_TOO_LARGE:
			return "Request Entity Too Large";
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
