#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../Head.hpp"
#include "../config/Config.hpp"
#include "../http/HttpStatus.hpp"
#include "../net/Socket.hpp"

#define MAX_URI_LENGTH 8192

struct MultipartField {
	std::string name;
	std::string filename;
	std::string contentType;
	std::string data;
};

class Request : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;
	typedef std::vector<MultipartField>		   MultipartFields;
	typedef std::map<std::string, std::string> FormData;

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
	long		_contentLength;
	ParseState	_parseState;
	bool		_hasCgi;
	std::string _resolveUri;
	std::string _resolveFullUri;

	MultipartFields _multipartFields;
	FormData		_formData;

  public:
	Request();
	Request(const Socket* servSocket, const std::string& clientIp);
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	bool parse(std::string& buffer);

	bool isComplete() const;
	bool isValid() const;
	bool hasCgi() const;

	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getresolveUri() const;
	const std::string& getresolveFullUri() const;
	const std::string& getVersion() const;
	const std::string& getBody() const;
	const std::string& getClientIp() const;
	std::string		   getHeader(const std::string& key) const;
	const HeaderMap&   getHeaders() const;
	const std::string& getResolvePath() const;
	const std::string& getResolveFullPath() const;
	long			   getContentLength() const;

	const LocationConfig* getLocationConf() const;
	const ServerConfig*	  getConf() const;
	const std::string&	  getServerPort() const;
	const std::string&	  getServerIp() const;
	const FormData&		  getFormData() const;

	const MultipartFields& getMultipartFields() const;
	void				   setFormData(const std::string& key, const std::string& val);

  private:
	void processLine(const std::string& line);
	void parseRequestLine(const std::string& line);
	void parseHeaderLine(const std::string& line);
	void parseMultipartHeaderLine(const std::string& line, MultipartField& field);

	void		parseMultipart(const std::string& boundary);
	void		parsePart(std::string& part);
	std::string extractParam(const std::string& headerValue, const std::string& param) const;
	void		parseUrlEncoded();

	bool matchLocation();
	void detectCgi();

	void setError(CodeStatus code);

	bool		isValidUri(const std::string& uri) const;
	std::string resolveFullPath() const;
};

// M U V
#endif
