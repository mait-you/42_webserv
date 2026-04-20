#ifndef HTTPSTATUS_HPP
#define HTTPSTATUS_HPP

#include <string>

class HttpStatus {
  public:
	enum CodeStatus {
		// 2xx Success
		HTTP_200_OK			= 200,
		HTTP_201_CREATED	= 201,
		HTTP_202_ACCEPTED	= 202,
		HTTP_204_NO_CONTENT = 204,

		// 3xx Redirection
		HTTP_301_MOVED_PERMANENTLY = 301,
		HTTP_302_FOUND			   = 302,
		HTTP_304_NOT_MODIFIED	   = 304,

		// 4xx Client Error
		HTTP_400_BAD_REQUEST			  = 400,
		HTTP_401_UNAUTHORIZED			  = 401,
		HTTP_403_FORBIDDEN				  = 403,
		HTTP_404_NOT_FOUND				  = 404,
		HTTP_405_METHOD_NOT_ALLOWED		  = 405,
		HTTP_413_REQUEST_ENTITY_TOO_LARGE = 413,

		// 5xx Server Error
		HTTP_500_INTERNAL_SERVER_ERROR = 500,
		HTTP_501_NOT_IMPLEMENTED	   = 501,
		HTTP_502_BAD_GATEWAY		   = 502,
		HTTP_503_SERVICE_UNAVAILABLE   = 503
	};

	enum HttpVersion { HTTP_0_9, HTTP_1_0, HTTP_1_1, HTTP_UNKNOWN };

  protected:
	CodeStatus	_statusCode;
	HttpVersion _httpV;

  public:
	HttpStatus();
	HttpStatus(CodeStatus statusCode);
	HttpStatus(const HttpStatus& other);
	HttpStatus& operator=(const HttpStatus& other);
	virtual ~HttpStatus();

	void setStatus(CodeStatus CodeStatus);

	CodeStatus	getStatusCode() const;
	std::string getStatusMessage() const;

	bool isError() const;
	bool isSuccess() const;

  private:
	std::string defaultMessage() const;
};

#endif
