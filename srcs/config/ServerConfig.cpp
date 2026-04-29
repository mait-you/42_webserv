#include "../../includes/config/Config.hpp"
#include "../../includes/utils/Utils.hpp"

bool dupLocation(std::vector<LocationConfig> locations, std::string path) {
	for (size_t i = 0; i < locations.size(); i++) {
		if (locations[i].path == path) {
			return true;
		}
	}
	return false;
}

void parselisten(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected port");
	}
	std::string	port;
	size_t		pos = 0;
	std::string	parsedHost = "0.0.0.0";

	pos = tokens[i].value.find(':');
	if (pos != std::string::npos) {
		parsedHost = tokens[i].value.substr(0, pos);
		if (parsedHost.empty())
			throw std::runtime_error("Invalid config: invalid host");
		port = tokens[i].value.substr(pos + 1);
	} else {
		port = tokens[i].value;
	}
	// if (!isNumber(port) || !isValidPort(port))
	// 	throw std::runtime_error("Invalid config: invalid port");
	for (size_t j = 0; j < server.listens.size(); j++) {
		if (server.listens[j].port == port && server.listens[j].host == parsedHost) {
			throw std::runtime_error("Invalid config: duplicate listen");
		}
	}
	Listen newListen;
	newListen.host = parsedHost;
	newListen.port = port;
	server.listens.push_back(newListen);
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseServerName(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected server name");
	}
	server.server_name = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseRoot(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected root");
	}
	server.root = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseServerIndex(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected index");
	}
	server.index = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseErrorPage(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected error page");
	}
	unsigned int	  errorCode;
	std::stringstream ss(tokens[i].value);
	ss >> errorCode;
	if (ss.fail() || !ss.eof()) {
		throw std::runtime_error("Invalid config:Expected error CodeStatus ");
	}
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected error page path");
	}
	server.error_pages[errorCode] = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseMaxSize(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected client max body size");
	}
	unsigned long	  value;
	std::string		  remaining;
	std::stringstream ss(tokens[i].value);
	ss >> value;
	if (ss.fail()) {
		throw std::runtime_error("Invalid config: client max body size not valid");
	}
	if (!ss.eof()) {
		ss >> remaining;
		if (remaining == "K" || remaining == "KB")
			value *= 1024;
		else if (remaining == "M" || remaining == "MB")
			value *= 1024 * 1024;
		else if (remaining == "G" || remaining == "GB")
			value *= 1024 * 1024 * 1024;
		else {
			throw std::runtime_error("Invalid config: client_max_body_size not valid");
		}
	}
	server.client_max_body_size = value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseLocation(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: unexpected location path");
	}
	LocationConfig location;

	if (dupLocation(server.locations, tokens[i].value)) {
		std::string dupPath = "Invalid config: duplicated location " + tokens[i].value;
		throw std::runtime_error(dupPath);
	}

	location.path = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != openBrace) {
		throw std::runtime_error("Invalid config: expected { after location");
	}
	i++;
	parseLocation(tokens, i, location);
	if (i >= tokens.size() || tokens[i].type != closeBrace) {
		throw std::runtime_error("Invalid config: expected } in location");
	}
	i++;
	if (!location.has_max)
		location.client_max_body_size = server.client_max_body_size;
	if (location.root.empty())
		location.root = server.root;
	if (location.index.empty())
		location.index = server.index;
	if (location.error_pages.empty())
		location.error_pages = server.error_pages;
	server.locations.push_back(location);
}

void parseServer(std::vector<Token>& tokens, size_t& i, ServerConfig& server) {
	size_t tokenSize = tokens.size();
	if (i >= tokenSize) {
		throw std::runtime_error("Invalid config: expected }");
	}
	while (i < tokenSize) {
		if (tokens[i].type == closeBrace) {
			return;
		}
		if (tokens[i].type == word && tokens[i].value == "listen")
			parselisten(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "server_name")
			parseServerName(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "root")
			parseRoot(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "index")
			parseServerIndex(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "error_page")
			parseErrorPage(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "client_max_body_size")
			parseMaxSize(i, tokens, server);
		else if (tokens[i].type == word && tokens[i].value == "location")
			parseLocation(i, tokens, server);
		else {
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
}
