#ifndef CGI_HPP
#define CGI_HPP

#include "Config.hpp"
#include "Head.hpp"
#include "Request.hpp"

class Cgi {
  private:
	const Request&		  _req;
	ServerConfig		  _srv;
	const LocationConfig* _loc;
	std::string			  _scriptPath;

  public:
	Cgi(const Request& req, const ServerConfig& srv, const LocationConfig* loc,
		const std::string& path);
	~Cgi();
	std::string run();

  private:
	std::vector<std::string> createEnv() const;

	Cgi();
	Cgi(const Cgi& other);
	Cgi& operator=(const Cgi& other);
};

#endif
