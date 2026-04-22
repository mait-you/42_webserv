#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../Head.hpp"

enum TokenType { word, openBrace, closeBrace, semiColone };

struct Token {
	TokenType	type;
	std::string	value;
};

struct Listen {
	std::string host;
	std::string port;
};

struct LocationConfig {
	std::string						 	path;
	std::vector<std::string>		 	allow_methods;
	bool							 	autoindex;
	bool							 	upload;
	std::string						 	upload_path;
	std::string						 	root;
	std::string						 	index;
	bool							 	has_redirect;
	std::string						 	redirect_url;
	int								 	redirect_code;
	std::map<int, std::string>		 	error_pages;
	bool							 	has_cgi;
	std::map<std::string, std::string>	cgi;
	bool								isAlias;
	bool								has_max;
	std::size_t							client_max_body_size;
	LocationConfig();
};

struct ServerConfig {
	std::vector<Listen>			listens;
	std::string					server_name;
	std::string					root;
	std::string					index;
	std::map<int, std::string>	error_pages;
	std::size_t					client_max_body_size;
	std::vector<LocationConfig> locations;
	ServerConfig();
};

class Config {
	private:
		std::vector<ServerConfig> _servers;

	public:
		Config();
		Config(const std::string& confFile);
		Config(const Config& other);
		Config& operator=(const Config& other);
		~Config();
		const std::vector<ServerConfig>& getServers() const;

	private:
		void parse(const std::string& confFile);
};

std::vector<Token>	tokenize(const std::string& filename);
void				parseLocation(std::vector<Token>& tokens, size_t& i, LocationConfig& location);
void				parseServer(std::vector<Token>& tokens, size_t& i, ServerConfig& server);

#endif
