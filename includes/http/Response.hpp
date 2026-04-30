#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../cgi/Cgi.hpp"
#include "../http/Request.hpp"

class Response : public HttpStatus {
  public:
	typedef std::map<std::string, std::string> HeaderMap;
	typedef HeaderMap::iterator				   HeaderIt;
	typedef HeaderMap::const_iterator		   ConstHeaderIt;

  private:
	HeaderMap	_headers;
	std::string _body;

	bool								_hasCgiRunning;
	CgiInfo								_runningCgi;
	std::map<std::string, std::string>* _sessions;
	bool								_isComplete;

  public:
	Response();
	Response(std::map<std::string, std::string>* session);
	Response(const Response& other);
	Response& operator=(const Response& other);
	void	  operator=(const Request& req);
	~Response();

	CodeStatus		   getStatusCode() const;
	const HeaderMap&   getHeaders() const;
	const std::string& getBody() const;

	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);

	bool isComplete() const;

	bool hasCgiRunning() const;

	std::string build(Request& request);
	bool		pollCgi(const Request& request);
	std::string buildSendBuffer();

  private:
	void handleRedirect(const Request& request);
	void handleByMethod(Request& request);

	bool		allowedMethods(const Request& request);
	std::string getList(const std::string& fullPath, const std::string& uri);

	void handleFile(const Request& request, const std::string& fullPath);
	void handleDir(const Request& request, const std::string& fullPath);
	int	 handleErrorFile(const std::string& fullPath);

	void deleteFolder(const Request& request, const std::string& fullPath);
	void errorPage(const Request& request, HttpStatus::CodeStatus code);

	void applyCgiHeaders(const std::string& rawHeaders, CodeStatus& outStatus,
						 std::string& outLocation, bool& outHasContentType);
	void processCgiOutput(const Request& request);

	void handleGet(const Request& request);
	void handleDelete(const Request& request);
	void handlePost(Request& request);
	void multiPart(Request& request, const MultipartField& part, std::string uploadDir);
	void handleLogin(const Request& request);

	void handleDashboard(const Request& request, const std::string& fullPath);
	void handleLogout(const Request& request);
};

#endif
