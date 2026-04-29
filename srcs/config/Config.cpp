#include "../../includes/config/Config.hpp"

Config::Config() {}

Config::Config(const std::string& confFile) {
	parse(confFile);
}

Config::Config(const Config& other) : _servers(other._servers) {}

Config& Config::operator=(const Config& other) {
	if (this != &other)
		_servers = other._servers;
	return *this;
}

Config::~Config() {}

ServerConfig::ServerConfig() {
	root				 = "./www";
	index				 = "index.html";
	client_max_body_size = 1 * 1024 * 1024;
}

LocationConfig::LocationConfig() {
	allow_methods.push_back("GET");
	autoindex			 = false;
	upload				 = false;
	has_redirect		 = false;
	redirect_code		 = 0;
	has_cgi				 = false;
	isAlias				 = false;
	has_max				 = false;
	client_max_body_size = 0;
}

bool hasSameHostPort(const ServerConfig& a, const ServerConfig& b) {
	for (size_t i = 0; i < a.listens.size(); i++) {
		for (size_t j = 0; j < b.listens.size(); j++) {
			if (a.listens[i].port == b.listens[j].port)
			{
				if (a.listens[i].host == b.listens[j].host)
					return true;
				else if (a.listens[i].host == "0.0.0.0" || b.listens[j].host == "0.0.0.0")
					return true;
			}
		}
	}
	return false;
}

void Config::parse(const std::string& filename) {
	std::vector<Token> tokens = tokenize(filename);

	size_t i = 0;
	while (i < tokens.size()) {
		if (tokens[i].type == word && tokens[i].value == "server") {
			i++;
			if (i >= tokens.size() || tokens[i].type != openBrace) {
				throw std::runtime_error("Invalid config: expected { after server");
			}
			i++;
			ServerConfig server;
			parseServer(tokens, i, server);
			if (server.listens.empty())
			{
				Listen defaultListen;
				defaultListen.host = "0.0.0.0";
				defaultListen.port = "8080";
				server.listens.push_back(defaultListen);
			}
			if (server.locations.empty()) {
				LocationConfig location;
				location.path = "/";
				server.locations.push_back(location);
			}
			for (size_t s = 0; s < _servers.size(); s++) {
				ServerConfig& current = _servers[s];
				if (hasSameHostPort(server, current)) {
						throw std::runtime_error("Invalid config: Invalid server config");
				}
			}
			if (i >= tokens.size() || tokens[i].type != closeBrace) {
				throw std::runtime_error("Invalid config: expected } in server block");
			}
			i++;
			_servers.push_back(server);
		} else {
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
	if (_servers.empty())
		throw std::runtime_error("Config file is empty or contains no server blocks");
}

const std::vector<ServerConfig>& Config::getServers() const {
	return _servers;
}
