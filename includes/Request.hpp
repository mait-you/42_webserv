#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Config.hpp"
#include "Head.hpp"

#define HTTP_VERSION "HTTP/1.1"
#define MAX_URI_LENGTH 8192
#define END_OF_HEADERS "\r\n\r\n"

class Request {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

	enum HttpError {
		OK					= 0,
		BAD_REQUEST			= 400,
		URI_TOO_LONG		= 414,
		BODY_TOO_LONG		= 413,
		UNSUPPORTED_VERSION = 505,
		NOT_IMPLEMENTED		= 501
	};

	enum ParseState { PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_COMPLETE };

  private:
	std::string _method;
	std::string _uri;
	std::string _version;
	HeaderMap	_headers;
	std::string _body;
	HttpError	_error;
	ParseState	_state;
	std::size_t _parsePos;
	std::size_t _bodyExpected;
	bool		_requestComplete;

  public:
	Request();
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	bool parse(const std::string& recvBuffer);

	// getters
	HttpError		 getError() const;
	bool			 isValid() const;
	bool			 isComplete() const;
	std::string		 getMethod() const;
	std::string		 getUri() const;
	std::string		 getVersion() const;
	std::string		 getBody() const;
	const HeaderMap& getHeaders() const;
	std::string		 getHeader(const std::string& key) const;

  private:
	void parseRequestLine(const std::string& buf);
	void parseHeaders(const std::string& buf);
	void parseBody(const std::string& buf);

	bool getLine(const std::string& buf, std::size_t& pos, std::string& line) const;

	std::string decodeChunked(const std::string& buf, std::size_t& pos) const;

	bool isValidMethod(const std::string& method) const;
	bool isValidUri(const std::string& uri) const;
	bool isValidVersion(const std::string& version) const;
	bool isValidHeaders() const;

	void setError(HttpError err);  // throws
	void setState(ParseState state);
};

std::ostream& operator<<(std::ostream& out, const Request& req);

#endif
