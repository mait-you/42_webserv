#include "../../includes/Config.hpp"

Config::Config() {
}

Config::Config(const std::string &confFile) {
	parse(confFile);
}

Config::Config(const Config &other) : _servers(other._servers) {
}

Config &Config::operator=(const Config &other) {
	if (this != &other)
		_servers = other._servers;
	return *this;
}

Config::~Config() {
}


ServerConfig::ServerConfig()
{
	ports.push_back("8080");
	host = "0.0.0.0";
	root = "./www";
	index = "index.html";
	client_max_body_size = 1 * 1024 * 1024;
}

LocationConfig::LocationConfig()
{
	allow_methods.push_back("GET");
	autoindex = false;
	upload = false;
	has_redirect = false;
	redirect_code = 0;
	has_cgi = false;
}

bool hasSamePort(const ServerConfig &a, const ServerConfig &b) {
	for (size_t i = 0; i < a.ports.size(); i++) {
		for (size_t j = 0; j < b.ports.size(); j++) {
			if (a.ports[i] == b.ports[j])
				return true;
		}
	}
	return false;
}

void Config::parse(const std::string &filename) {
	std::vector<Token> tokens = tokenize(filename);

	size_t i = 0;
	while ( i < tokens.size())
	{
		if (tokens[i].type == word && tokens[i].value == "server")
		{
			i++;
			if (i >= tokens.size() || tokens[i].type != openBrace)
			{
				throw std::runtime_error("Invalid config: expected { after server");
			}
			i++;
			ServerConfig server;
			parseServer(tokens, i, server);
		
			for (size_t s = 0; s < _servers.size(); s++) {
				ServerConfig &current = _servers[s];
				if (hasSamePort(server, current)) {
					if (server.server_name == current.server_name ||
						server.host == current.host) {
						throw std::runtime_error("Invalid config: Invalid server config");
					}
				}
			}
			
			_servers.push_back(server);
		}
		else
		{
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
	if (_servers.empty())
		throw std::runtime_error("Config file is empty or contains no server blocks");
}

const std::vector<ServerConfig> &Config::getServers() const {
	return _servers;
}

std::ostream &operator<<(std::ostream &out, const LocationConfig &loc) {
	out << "    Location: " << loc.path << std::endl;
	out << "      Methods: ";
	for (size_t i = 0; i < loc.allow_methods.size(); i++) {
		out << loc.allow_methods[i];
		if (i + 1 < loc.allow_methods.size())
			out << ", ";
	}
	out << std::endl;
	out << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
	out << "      Upload: " << (loc.upload ? "on" : "off") << std::endl;
	if (loc.upload)
		out << "      Upload Path: " << loc.upload_path << std::endl;
	if (!loc.root.empty())
		out << "      Root: " << loc.root << std::endl;
	if (!loc.index.empty())
		out << "      Index: " << loc.index << std::endl;
	if (loc.has_redirect) {
		out << "      Redirect: " << loc.redirect_code << " "
			<< loc.redirect_url << std::endl;
	}

	out << "      has_cgi: " << (loc.has_cgi ? "on" : "off") << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin(); it != loc.cgi.end(); ++it)
		out << "      cgi_pass: " << it->first << " " << it->second << std::endl;
	return out;
}

std::ostream &operator<<(std::ostream &out, const ServerConfig &server) {
	out << "  Server:" << std::endl;
	out << "    Host: " << server.host << std::endl;
	out << "    Ports: ";
	for (size_t i = 0; i < server.ports.size(); i++) {
		out << server.ports[i];
		if (i + 1 < server.ports.size())
			out << ", ";
	}
	out << std::endl;
	if (!server.server_name.empty())
		out << "    Server Name: " << server.server_name << std::endl;
	out << "    Root: " << server.root << std::endl;
	out << "    Index: " << server.index << std::endl;
	// if (!server.error_page.empty())
	// 	out << "    Error Page: " << server.error_page << std::endl;
	out << "    Max Body Size: " << server.client_max_body_size << " bytes"
		<< std::endl;

	if (!server.locations.empty()) {
		out << "    Locations:" << std::endl;
		for (size_t i = 0; i < server.locations.size(); i++) {
			out << server.locations[i];
		}
	}
	return out;
}

std::ostream &operator<<(std::ostream &out, const Config &config) {
	const std::vector<ServerConfig> &servers = config.getServers();

	out << "Configuration:" << std::endl;
	out << "Total Servers: " << servers.size() << std::endl;
	out << std::endl;

	for (size_t i = 0; i < servers.size(); i++) {
		out << "Server [" << i + 1 << "]:" << std::endl;
		out << servers[i];
		out << std::endl;
	}

	return out;
}
