#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "head.hpp"

struct LocationConfig {
	std::string				 path; // location path
	std::vector<std::string> allow_methods;
	bool					 autoindex;
	bool					 upload;
	std::string				 upload_path;
	std::string				 root;	// root for this location
	std::string				 index; // default file
	bool					 has_redirect;
	std::string				 redirect_url;
	int						 redirect_code; // 301 or 302

	// bool					 has_cgi;
	// std::string				 cgi_extension; // ".php"
	// std::string				 cgi_path;		// "/usr/bin/php-cgi"
};

struct ServerConfig {
	std::vector<std::string>	ports;
	std::string					host;
	std::string					server_name; // NEW
	std::string					root;
	std::string					index;
	std::string					error_page;
	unsigned long				client_max_body_size;
	std::vector<LocationConfig> locations;
};

class Config {
  private:
	std::vector<ServerConfig> _servers; // multiple server blocks

  public:
	Config();
	~Config();

	void							 parse(const std::string &filename);
	const std::vector<ServerConfig> &getServers() const;

  private:
	Config(const Config &other);
	Config &operator=(const Config &other);
};

std::ostream &operator<<(std::ostream &out, const Config &config);
std::ostream &operator<<(std::ostream &out, const LocationConfig &loc);
std::ostream &operator<<(std::ostream &out, const ServerConfig &server);

#endif
