#ifndef CGI_HPP
#define CGI_HPP

#include "../Head.hpp"
#include "../config/Config.hpp"
#include "../http/Request.hpp"

struct CgiInfo {
	pid_t		pid;
	std::string resPath;
	std::string bodyPath;
	std::time_t startTime;
	CgiInfo();
};

class Cgi {
  private:
	Request				  _req;
	ServerConfig		  _srv;
	const LocationConfig* _loc;
	std::string			  _scriptPath;
	std::string			  _resPath;
	std::string			  _bodyPath;

  public:
	Cgi(const Request& req, const ServerConfig& srv, const LocationConfig* loc,
		const std::string& path);
	~Cgi();
	CgiInfo start();

  private:
	Cgi();
	Cgi(const Cgi&);
	Cgi& operator=(const Cgi&);

	std::vector<std::string> createEnv() const;
	const std::string		 findCgiPath() const;
	int						 createFiles();
};

#endif
