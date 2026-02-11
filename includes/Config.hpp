#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "webserv.hpp"

struct LocationConfig {
	std::string				 path;			// location path (/, /upload)
	std::vector<std::string> allow_methods; // GET, POST, DELETE
	bool					 autoindex;		// on/off
	bool					 upload;		// on/off
	std::string				 upload_path;	// path for uploads
};

struct ServerConfig {
	std::vector<std::string>	ports;				  // listening ports
	std::string					host;				  // host address
	std::string					root;				  // website root
	std::string					index;				  // index file
	std::string					error_page;			  // error page path
	unsigned long				client_max_body_size; // max body size in bytes
	std::vector<LocationConfig> locations;			  // location blocks
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


#endif
