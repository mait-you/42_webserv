#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../Head.hpp"
#include "../config/Config.hpp"
#include "../http/HttpStatus.hpp"
#include "../net/Socket.hpp"

#define MAX_URI_LENGTH 8192

/* RFC 7578 §4.2 — each part carries a name, optional filename,
   optional Content-Type, and the raw part data */
struct MultipartField {
	std::string name;		 /* "name" param of Content-Disposition   */
	std::string filename;	 /* "filename" param — empty if not a file */
	std::string contentType; /* Content-Type of part, default text/plain (§4.4) */
	std::string data;		 /* raw bytes of this part's body          */
};

class Request : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

	/* RFC 7578 §4.3 — multiple parts may share the same field name,
	   so we store a list, not a map */
	typedef std::vector<MultipartField> MultipartFields;

	enum ParseState { PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_DONE, PARSE_ERROR };

  private:
	const ServerConfig*	  _srvConf;
	const LocationConfig* _locConf;
	std::string			  _method;
	std::string			  _uri;
	std::string			  _version;
	HeaderMap			  _headers;
	std::string			  _body;

	std::string _serverPort;
	std::string _serverIp;
	std::string _clientIp;
	std::size_t _contentLength;
	ParseState	_parseState;
	bool		_hasCgi;
	std::string _resolveUri;
	std::string _resolveFullUri;

	/* RFC 7578 §4 — parsed multipart parts, filled by parseMultipart() */
	MultipartFields _multipartFields;

  public:
	Request();
	Request(const Socket* servSocket, const std::string& clientIp);
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	bool parse(std::string& buffer);

	/* Getters */
	bool isComplete() const;
	bool isValid() const;
	bool hasCgi() const;

	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getVersion() const;
	const std::string& getBody() const;
	const std::string& getClientIp() const;
	std::string		   getHeader(const std::string& key) const;
	const HeaderMap&   getHeaders() const;
	const std::string& getResolvePath() const;
	const std::string& getResolveFullPath() const;

	const LocationConfig* getLocationConf() const;
	const ServerConfig*	  getConf() const;
	const std::string&	  getServerPort() const;
	const std::string&	  getServerIp() const;

	/* RFC 7578 — access parsed multipart fields */
	const MultipartFields& getMultipartFields() const;

  private:
	void processLine(const std::string& line);
	void parseRequestLine(const std::string& line);
	void parseHeaderLine(const std::string& line);

	/* RFC 7578 §4 — multipart parsing pipeline */
	void		parseMultipart(const std::string& boundary);
	bool		parsePartHeaders(const std::string& headerBlock, MultipartField& field) const;
	std::string extractParam(const std::string& headerValue, const std::string& param) const;

	bool matchLocation();
	void detectCgi();

	void setError(CodeStatus code);

	bool		isValidUri(const std::string& uri) const;
	std::string resolveFullPath() const;
};

#endif
