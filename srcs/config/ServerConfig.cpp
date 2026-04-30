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
		if (server.listens[j].port == port &&
			(server.listens[j].host == "0.0.0.0" || parsedHost == "0.0.0.0")) {
			throw std::runtime_error("Invalid config: invalid listen");
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
	if (server.hasServerName)
		throw std::runtime_error("Invalid config: Duplicate root");
	server.server_name = tokens[i].value;
	server.hasServerName = true;
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
	if (server.hasRoot)
		throw std::runtime_error("Invalid config: Duplicate root");
	server.root = tokens[i].value;
	server.hasRoot = true;
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
	if (server.hasIndex)
		throw std::runtime_error("Invalid config: Duplicate index");
	server.index = tokens[i].value;
	server.hasIndex = true;
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
	long	errorCode;
	std::stringstream ss(tokens[i].value);
	ss >> errorCode;
	if (ss.fail() || !ss.eof() || errorCode < 0) {
		throw std::runtime_error("Invalid config:Expected error CodeStatus ");
	}
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected error page path");
	}
	if(server.error_pages.count(errorCode))
	{
		std::string str = "Invalid config: Duplicate error page " + tokens[i].value;
		throw std::runtime_error(str);
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
	long		value;
	long		tmp;
	std::string	remaining;
	std::stringstream ss(tokens[i].value);
	ss >> value;
	if (ss.fail() || value < 0) {
		throw std::runtime_error("Invalid config: client max body size not valid");
	}
	if (!ss.eof()) {
		ss >> remaining;
		if (remaining == "K" || remaining == "KB")
			tmp = value * 1024;
		else if (remaining == "M" || remaining == "MB")
			tmp = value * 1024 * 1024;
		else if (remaining == "G" || remaining == "GB")
			tmp = value * 1024 * 1024 * 1024 ;
		else {
			throw std::runtime_error("Invalid config: client_max_body_size not valid");
		}
		if (tmp < value)
			throw std::runtime_error("Invalid config: client_max_body_size not valid");
	}
	if (server.hasMax)
		throw std::runtime_error("Invalid config: Duplicate client_max_body_size");
	if (tmp > 100 * 1024 * 1024)
		throw std::runtime_error("Invalid config: Duplicate client_max_body_size");
	server.client_max_body_size = tmp;
	server.hasMax = true;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseLocationBlock(size_t& i, std::vector<Token>& tokens, ServerConfig& server) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: unexpected location path");
	}
	LocationConfig location;

	std::string path = tokens[i].value;
	if (path[0] != '/')
	{
		std::string dupPath = "Invalid config: invalid location " + tokens[i].value;
		throw std::runtime_error(dupPath);
	}
	std::stringstream ss (path);
	std::string part;
	std::vector<std::string> res;
	while (std::getline(ss, part, '/'))
	{
		if (!part.empty())
			res.push_back(part);
	}
	std::string str = "/";
	for (size_t i = 0; i < res.size(); i++)
	{
		str += res[i];
		if (i + 1 < res.size())
			str += "/";
	}
	if (dupLocation(server.locations, str)) {
		std::string dupPath = "Invalid config: duplicated location " + tokens[i].value;
		throw std::runtime_error(dupPath);
	}
	location.path = str;
	i++;
	if (i >= tokens.size() || tokens[i].type != openBrace) {
		throw std::runtime_error("Invalid config: expected { after location");
	}
	i++;
	parseLocationBody(tokens, i, location);
	if (i >= tokens.size() || tokens[i].type != closeBrace) {
		throw std::runtime_error("Invalid config: expected } in location");
	}
	i++;
	if (!location.hasMax)
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
			parseLocationBlock(i, tokens, server);
		else {
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
}
