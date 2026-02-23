#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "head.hpp"

#define END_OF_HEADERS "\r\n\r\n"
#define MAX_URI_LENGTH 8192
#define MAX_BODY_SIZE (1024 * 1024)

class Request {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

	enum HttpError {
		OK					= 0,
		BAD_REQUEST			= 400,
		UNSUPPORTED_VERSION = 505,
	};

  private:
	std::string _method;
	std::string _uri;
	std::string _version;
	HeaderMap	_headers;
	std::string _body;
	HttpError	_error;

  public:
	Request();
	Request(const Request &other);
	Request &operator=(const Request &other);
	~Request();

	bool parse(const std::string &buffer);
	void validate();

	HttpError		 getError() const;
	bool			 isValid() const;
	std::string		 getMethod() const;
	std::string		 getUri() const;
	std::string		 getVersion() const;
	std::string		 getBody() const;
	const HeaderMap &getHeaders() const;
	std::string		 getHeader(const std::string &key) const;

  private:
	bool		isValidMethod(const std::string &method) const;
	bool		isValidUriChars(const std::string &uri) const;
	std::string parseChunkedBody(const std::string &raw, std::size_t pos);
};

std::ostream &operator<<(std::ostream &out, const Request &request);

#endif
