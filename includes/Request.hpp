#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Config.hpp"
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
	const ServerConfig*	  _srvConf;
	const LocationConfig* _locConf;
	std::string			  _method;
	std::string			  _uri;
	std::string			  _version;
	HeaderMap			  _headers;
	std::string			  _body;

	ParseState	_state;
	std::size_t _parsePos;
	bool		_hasCgi;

  public:
	Request();
	Request(const ServerConfig* serverConfig);
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	bool parse(const std::string& recvBuffer);

	// getters
	bool				  isValid() const;
	bool				  isComplete() const;
	std::string			  getMethod() const;
	std::string			  getUri() const;
	std::string			  getVersion() const;
	std::string			  getBody() const;
	const HeaderMap&	  getHeaders() const;
	std::string			  getHeader(const std::string& key) const;
	const LocationConfig* getLocationConf() const;
	const ServerConfig*	  getServerConf() const;
	bool				  hasCgi() const;
	std::string			  resolvePath() const;
	std::string			  resolveFullPath() const;

  private:
	bool parseRequestLine(const std::string& buf);
	bool parseHeaders(const std::string& buf);
	bool parseBody(const std::string& buf);

	bool isValidMethod(const std::string& method) const;
	bool isValidUri(const std::string& uri) const;
	bool isValidVersion(const std::string& version) const;
	bool isValidHeaders() const;

	void detectCgi();

	bool setError(codeStatus code);  // throws
	void setState(ParseState state);

	bool matchedLocation();
};
void	  printRequest(std::ostream& out, const Request& req, const std::string& pre,
						   const std::string& last);
std::ostream& operator<<(std::ostream& out, const Request& req);

#endif
