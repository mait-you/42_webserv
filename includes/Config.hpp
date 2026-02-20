#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "head.hpp"

enum TokenType {
	word,
	openBrace,
	closeBrace,
	semiColone
};

struct Token {
	TokenType type;
	std::string value;
};

struct LocationConfig {
	std::string				 path;
	std::vector<std::string> allow_methods;
	bool					 autoindex;
	bool					 upload;
	std::string				 upload_path;
	std::string				 root;
	std::string				 index;
	bool					 has_redirect;
	std::string				 redirect_url;
	int						 redirect_code; // 301 or 302
	std::map<int, std::string> error_pages;

	bool					 has_cgi;
	std::string				 cgi_extension; // ".php"
	std::string				 cgi_path; // "/usr/bin/php-cgi"
	LocationConfig();
};

struct ServerConfig {
	std::vector<std::string>	ports;
	std::string					host;
	std::string					server_name;
	std::string					root;
	std::string					index;
	// std::string					error_page;
	std::map<int, std::string> error_pages;
	unsigned long				client_max_body_size;
	std::vector<LocationConfig> locations;
	ServerConfig();
};

class Config {
  private:
	std::vector<ServerConfig> _servers;

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

std::vector<Token> tokenize(const std::string &filename);
void parseLocation(std::vector<Token> &tokens, size_t &i, LocationConfig &location);
void parseServer(std::vector<Token> &tokens, size_t &i, ServerConfig &server);

#endif
