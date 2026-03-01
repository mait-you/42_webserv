#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Config.hpp"
#include "Head.hpp"
#include "HttpStatus.hpp"

#define MAX_URI_LENGTH 8192
#define END_OF_HEADERS "\r\n\r\n"

class Request : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

	enum ParseState { PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_COMPLETE };

  private:
	std::string _method;
	std::string _uri;
	std::string _version;
	HeaderMap	_headers;
	std::string _body;

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

	bool isValidMethod(const std::string& method) const;
	bool isValidUri(const std::string& uri) const;
	bool isValidVersion(const std::string& version) const;
	bool isValidHeaders() const;

	void setError(Code code);  // throws
	void setState(ParseState state);
};

std::ostream& operator<<(std::ostream& out, const Request& req);

#endif
