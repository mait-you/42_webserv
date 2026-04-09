#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Cgi.hpp"
#include "Request.hpp"
#include "SessionInfo.hpp"

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

	bool								_hasCgiRunning;
	CgiInfo								_runningCgi;
	std::map<std::string, SessionInfo>* _sessions;
	bool								_responseReady;
	bool								_isComplete;

  public:
	Response();
	Response(std::map<std::string, SessionInfo>* session);
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	codeStatus		   getStatusCode() const;
	const std::string& getStatusMessage() const;
	const HeaderMap&   getHeaders() const;
	const std::string& getBody() const;

	void setStatus(codeStatus codeStatus);
	void setStatus(codeStatus codeStatus, const std::string& message);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);

	bool isComplete() const;

	bool hasCgiRunning() const;

	std::string build(Request& request);
	bool		checkCgi(const Request& request);

  private:
	std::string buildSendBuffer();

	// helpers
	bool		allowedMethods(const Request& request);
	std::string getList(const std::string& fullPath, const std::string& uri);

	void handleFile(const Request& request, const std::string& fullPath);
	void handleDir(const Request& request, const std::string& fullPath);
	int	 handleErrorFile(const std::string& fullPath);

	void deleteFolder(const Request& request, const std::string& fullPath);
	void errorPage(const Request& request, codeStatus codeStatus);

	void parseCgiHeaders(const std::string& headers, codeStatus& status, std::string& msgStatus);

	void handleGet(const Request& request);
	void handleDelete(const Request& request);
	void handlePost(const Request& request);

	int handleDashboard(const Request& request, const std::string& fullPath);
	int handleLogout(const Request& request);
};

void	  printResponse(std::ostream& out, const Response& res, const std::string& pre,
							const std::string& last);
std::ostream& operator<<(std::ostream& out, const Response& res);

#endif
