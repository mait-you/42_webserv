#include "../../includes/Config.hpp"

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
	host				 = "0.0.0.0";
	root				 = "./www";
	index				 = "index.html";
	client_max_body_size = 1 * 1024 * 1024;
}

LocationConfig::LocationConfig() {
	allow_methods.push_back("GET");
	autoindex		= false;
	upload			= false;
	has_redirect	= false;
	redirect_code	= 0;
	has_cgi			= false;
	isAlias			= false;
}

bool hasSamePort(const ServerConfig& a, const ServerConfig& b) {
	for (size_t i = 0; i < a.ports.size(); i++) {
		for (size_t j = 0; j < b.ports.size(); j++) {
			if (a.ports[i] == b.ports[j])
				return true;
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
			if (server.ports.empty())
				server.ports.push_back("8080");
			if (server.locations.empty()) {
				LocationConfig location;
				location.path = "/";
				server.locations.push_back(location);
			}
			for (size_t s = 0; s < _servers.size(); s++) {
				ServerConfig& current = _servers[s];
				if (hasSamePort(server, current)) {
					if (server.host == current.host) {
						throw std::runtime_error("Invalid config: Invalid server config");
					}
					if (server.host == "0.0.0.0" || current.host == "0.0.0.0")
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

std::ostream& operator<<(std::ostream& out, const LocationConfig& loc) {
	out << GRY "│    " RST << CYN << loc.path << RST "\n";

	out << GRY "│      " RST "methods: ";
	for (size_t i = 0; i < loc.allow_methods.size(); i++) {
		out << GRN << loc.allow_methods[i] << RST;
		if (i + 1 < loc.allow_methods.size())
			out << GRY ", " RST;
	}
	out << "\n";

	if (!loc.root.empty())
		out << GRY "│      " RST "root:      " << WHT << loc.root << RST "\n";
	if (!loc.index.empty())
		out << GRY "│      " RST "index:     " << WHT << loc.index << RST "\n";
	if (loc.has_redirect)
		out << GRY "│      " RST "redirect:  " << YEL << loc.redirect_code << " "
			<< loc.redirect_url << RST "\n";

	out << GRY "│      " RST "autoindex: " << (loc.autoindex ? GRN "on" : GRY "off") << RST "\n";
	out << GRY "│      " RST "upload:    " << (loc.upload ? GRN "on" : GRY "off") << RST;
	if (loc.upload)
		out << "  " << WHT << loc.upload_path << RST;
	out << "\n";

	if (loc.has_cgi) {
		for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin();
			 it != loc.cgi.end(); ++it)
			out << GRY "│      " RST "cgi:       " << CYN << it->first << RST " -> " << WHT
				<< it->second << RST "\n";
	}

	return out;
}

std::ostream& operator<<(std::ostream& out, const ServerConfig& server) {
	out << GRY "│  " RST << WHT << server.host << RST;

	out << GRY ":" RST;
	for (size_t i = 0; i < server.ports.size(); i++) {
		out << YEL << server.ports[i] << RST;
		if (i + 1 < server.ports.size())
			out << GRY "," RST;
	}

	if (!server.server_name.empty())
		out << "  " << GRY "[" RST << server.server_name << GRY "]" RST;

	out << "\n";
	out << GRY "│    " RST "root:      " << WHT << server.root << RST "\n";
	out << GRY "│    " RST "index:     " << WHT << server.index << RST "\n";
	out << GRY "│    " RST "max body:  " << YEL << server.client_max_body_size << RST " bytes\n";

	if (!server.locations.empty()) {
		out << GRY "│    " RST "locations:\n";
		for (size_t i = 0; i < server.locations.size(); i++)
			out << server.locations[i];
	}

	return out;
}

std::ostream& operator<<(std::ostream& out, const Config& config) {
	const std::vector<ServerConfig>& servers = config.getServers();

	out << "\n" GRY "┌─ Config " RST << GRY "[" RST << servers.size()
		<< GRY "]" RST "\n";

	for (size_t i = 0; i < servers.size(); i++) {
		out << GRY "│ " RST << WHT "Server" RST << GRY " [" RST << (i + 1) << GRY "]" RST "\n";
		out << servers[i];
	}

	out << GRY "└─── ─ ─ ─ " RST "\n";
	return out;
}
