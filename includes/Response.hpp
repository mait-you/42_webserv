#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Config.hpp"
#include "Head.hpp"
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

  public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &other);
	~Response();

	void setStatus(codeStatus codeStatus);
	void setStatus(codeStatus codeStatus, const std::string &message);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);

	std::string build(Request &request, const std::vector<ServerConfig> &servers);

  private:
	std::string buildSendBuffer() const;

	// helpers
	ServerConfig	matchedServer(Request &req, const std::vector<ServerConfig> &servers);
	LocationConfig *matchedLocation(ServerConfig &srv, Request &req);
	bool			allowedMethods(LocationConfig *locConfig, Request &req);
	bool			bodySize(ServerConfig &srv, Request &req);
	std::string		getExtension(const std::string &fullPath);
	std::string		getList(const std::string &fullPath, const std::string &uri);
	std::string		cleanUri(std::string uri);
	void handleFile(ServerConfig &srv, LocationConfig *locConfig, const std::string &fullPath);
	int	 handleErrorFile(const std::string &fullPath);
	void handleDir(Request &req, ServerConfig &srv, LocationConfig *locConfig,
				   const std::string &fullPath);
	void deleteFolder(const std::string &fullPath, ServerConfig &srv, LocationConfig *locConfig);
	void errorPage(ServerConfig &srv, LocationConfig *locConfig, codeStatus codeStatus);

	void handleGet(Request &req, ServerConfig &srv, LocationConfig *locConfig);
	void handleDelete(Request &req, ServerConfig &srv, LocationConfig *locConfig);
	void handlePost(Request &, ServerConfig &srv, LocationConfig *locConfig);
};

#endif
