#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Cgi.hpp"
#include "Request.hpp"


class Response : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

  private:
	codeStatus						   _statusCode;
	std::string						   _statusMessage;
	std::map<std::string, std::string> _headers;
	std::string						   _body;

	bool		_hasCgiRunning;
	CgiInfo		_runningCgi;

  public:
	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	void setStatus(codeStatus codeStatus);
	void setStatus(codeStatus codeStatus, const std::string& message);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);

	bool hasCgiRunning() const;

	std::string build(Request &request);

  private:
	std::string buildSendBuffer() const;

	// helpers
	bool		allowedMethods(const Request& request);
	bool		bodySize(const Request& request);
	std::string getList(const std::string& fullPath, const std::string& uri);

	void handleFile(const Request& request, const std::string& fullPath);
	void handleDir(const Request& request, const std::string& fullPath);
	int	 handleErrorFile(const std::string& fullPath);

	void deleteFolder(const Request& request, const std::string& fullPath);
	void errorPage(const Request& request, codeStatus codeStatus);

	void handleGet(const Request& request);
	void handleDelete(const Request& request);
	void handlePost(const Request& request);
};

#endif
