#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../Head.hpp"
#include "../config/Config.hpp"
#include "../http/HttpStatus.hpp"

/*
 * RFC 1945 §3.2 — URI length is not specified, 8192 is a safe server limit.
 * RFC 1945 §5   — A Request is a Request-Line, headers, and optional body.
 */
#define MAX_URI_LENGTH 8192

class Request : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

	enum ParseState { PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_DONE, PARSE_ERROR };

  private:
	const ServerConfig*	  _srvConf;
	const LocationConfig* _locConf;

	std::string _method;
	std::string _uri;
	std::string _version;
	HeaderMap	_headers;
	std::string _body;
	std::string _clientIp;
	std::size_t _contentLength;

	ParseState _parseState;
	bool	   _hasCgi;

  public:
	Request();
	Request(const ServerConfig* srvConf, const std::string& clientIp);
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	/*
	 * RFC 1945 §5 — Feed raw received bytes. Returns true when the request
	 * is fully parsed (PARSE_DONE or PARSE_ERROR).
	 * The buffer is consumed as lines are extracted.
	 */
	bool parse(std::string& buffer);

	/* Getters */
	bool isComplete() const;
	bool isValid() const;
	bool hasCgi() const;

	std::string		 getMethod() const;
	std::string		 getUri() const;
	std::string		 getVersion() const;
	std::string		 getBody() const;
	std::string		 getClientIp() const;
	std::string		 getHeader(const std::string& key) const;
	const HeaderMap& getHeaders() const;

	const LocationConfig* getLocationConf() const;
	const ServerConfig*	  getConf() const;

	/*
	 * Path utilities — public because Response also needs them.
	 */
	std::string resolvePath() const;	 /* URI → clean relative path        */
	std::string resolveFullPath() const; /* clean path + root → full fs path */

  private:
	/* --- parsing pipeline --- */
	void processLine(const std::string& line);

	/*
	 * RFC 1945 §5.1   — Request-Line = Method SP Request-URI SP HTTP-Version CRLF
	 */
	void parseRequestLine(const std::string& line);

	/*
	 * RFC 1945 §4.2   — Message-Header = field-name ":" [ field-value ]
	 */
	void parseHeaderLine(const std::string& line);

	/* --- validation helpers --- */

	/*
	 * RFC 1945 §3.2   — URI must be non-empty printable US-ASCII, no CTL chars.
	 */
	bool isValidUri(const std::string& uri) const;

	/* --- location and CGI --- */
	bool matchLocation();
	void detectCgi();

	/* Mark the request as failed and stop parsing. */
	void setError(CodeStatus code);
};

#endif 
