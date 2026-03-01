#ifndef HTTPSTATUS_HPP
#define HTTPSTATUS_HPP

#include <string>

class HttpStatus {
  public:
	enum Code {
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
		HTTP_400_BAD_REQUEST  = 400,
		HTTP_401_UNAUTHORIZED = 401,
		HTTP_403_FORBIDDEN	  = 403,
		HTTP_404_NOT_FOUND	  = 404,

		// 5xx Server Error
		HTTP_500_INTERNAL_SERVER_ERROR = 500,
		HTTP_501_NOT_IMPLEMENTED	   = 501,
		HTTP_502_BAD_GATEWAY		   = 502,
		HTTP_503_SERVICE_UNAVAILABLE   = 503
	};

  protected:
	Code		_statusCode;
	std::string _statusMessage;

  public:
	HttpStatus();
	HttpStatus(Code code, const std::string& message);
	HttpStatus(const HttpStatus& other);
	HttpStatus& operator=(const HttpStatus& other);
	virtual ~HttpStatus();

	Code		getStatusCode() const;
	std::string getStatusMessage() const;
	void		setStatus(Code code, const std::string& message);
	bool		isError() const;
	bool		isSuccess() const;

	static std::string defaultMessage(Code code);
};

#endif
