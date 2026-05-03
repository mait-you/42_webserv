#include "../../includes/http/HttpStatus.hpp"

HttpStatus::HttpStatus() : _statusCode(HTTP_000_NO_CODE_STATUS), _httpVersion(HTTP_UNKNOWN) {}

HttpStatus::HttpStatus(CodeStatus statusCode) : _statusCode(statusCode), _httpVersion(HTTP_1_0) {}

HttpStatus::HttpStatus(const HttpStatus& other)
		: _statusCode(other._statusCode), _httpVersion(other._httpVersion) {}

HttpStatus& HttpStatus::operator=(const HttpStatus& other) {
	if (this != &other) {
		_statusCode	 = other._statusCode;
		_httpVersion = other._httpVersion;
	}
	return *this;
}

HttpStatus::~HttpStatus() {}

HttpStatus::CodeStatus HttpStatus::getStatusCode() const {
	return _statusCode;
}

std::string HttpStatus::getStatusMessage() const {
	switch (_statusCode) {
		case HTTP_200_OK:
			return "OK";
		case HTTP_201_CREATED:
			return "Created";
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
		case HTTP_403_FORBIDDEN:
			return "Forbidden";
		case HTTP_404_NOT_FOUND:
			return "Not Found";
		case HTTP_405_METHOD_NOT_ALLOWED:
			return "Method Not Allowed";
		case HTTP_500_INTERNAL_SERVER_ERROR:
			return "Internal Server Error";
		case HTTP_501_NOT_IMPLEMENTED:
			return "Not Implemented";
		default:
			return "Unknown";
	}
}

std::string HttpStatus::getHttpVersion() const {
	switch (_httpVersion) {
		case HTTP_0_9:
			return "HTTP/0.9";
		case HTTP_1_0:
			return "HTTP/1.0";
		case HTTP_1_1:
			return "HTTP/1.1";
		default:
			return "Unknown";
	}
}

void HttpStatus::setStatus(CodeStatus CodeStatus) {
	if (_httpVersion != HTTP_0_9)
		_statusCode = CodeStatus;
}

void HttpStatus::setVersion(HttpVersion httpVersion) {
	_httpVersion = httpVersion;
}

bool HttpStatus::isError() const {
	if (_httpVersion == HTTP_0_9)
		return true;
	return _statusCode >= HTTP_400_BAD_REQUEST && _statusCode <= HTTP_501_NOT_IMPLEMENTED;
}

bool HttpStatus::isSuccess() const {
	if (_httpVersion == HTTP_0_9)
		return true;
	return _statusCode >= HTTP_200_OK && _statusCode <= HTTP_204_NO_CONTENT;
}
