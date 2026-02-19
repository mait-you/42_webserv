#include "../../includes/Config.hpp"

Config::Config() {
	LOG_DEBUGG("Invalid config: ", "Default constructor called");
}

Config::Config(const Config &) {
	LOG_DEBUGG("Invalid config: ", "Copy constructor called");
}

Config &Config::operator=(const Config &) {
	LOG_DEBUGG("Invalid config: ", "Copy assignment constructor called");
	return *this;
}

Config::~Config() {
	LOG_DEBUGG("Invalid config: ", "Destructor constructor called");
}


ServerConfig::ServerConfig()
{
	ports.push_back("8080");
	host = "0.0.0.0";
	root = "./www";
	index = "index.html";
	client_max_body_size = 1 * 1080 * 1080;
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


std::vector<Token> tokenize(const std::string &filename)
{
	char c;
	struct Token token;
	std::vector<Token> tokens;

	std::ifstream configFile(filename.c_str());
	if (!configFile.is_open())
	{
		throw std::runtime_error("Error: cannot open config file");
	}
	while (configFile.get(c))
	{
		if (isspace(c))
			continue;
		if (c == '#')
		{
			while (configFile.get(c))
			{
				if (c == '\n')
					break;
			}
			continue;
		}
		if (c == '{')
		{
			token.type = openBrace;
			token.value = "{";
			tokens.push_back(token);
			continue;
		}
		if (c == '}')
		{
			token.type = closeBrace;
			token.value = "}";
			tokens.push_back(token);
			continue;
		}
		if (c == ';')
		{
			token.type = semiColone;
			token.value = ";";
			tokens.push_back(token);
			continue;
		}
		token.type = word;
		token.value = c;
	
		while (configFile.peek() != EOF &&
				!isspace(configFile.peek()) &&
				configFile.peek() != '{' &&
				configFile.peek() != '}' &&
				configFile.peek() != ';')
		{
			configFile.get(c);
			token.value += c;
		}
		tokens.push_back(token);
	}
	configFile.close();
	return tokens;
}

void parseLocation(std::vector<Token> &tokens, size_t &i, LocationConfig &location)
{
		size_t tokenSize = tokens.size();
		if (i >= tokenSize)
		{
			throw std::runtime_error("Invalid config: expected }");
		}
		while (i < tokenSize)
		{
			if (tokens[i].type == closeBrace)
			{
				i++;
				return;
			}
			else if (tokens[i].type == word && tokens[i].value == "index")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{
					throw std::runtime_error("Invalid config: Expected auto index");
					
				}
				location.upload_path = tokens[i].value;
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{					
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}
			else if (tokens[i].type == word && tokens[i].value == "upload_path")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{
					throw std::runtime_error("Invalid config: Expected upload path");
				}
				location.upload_path = tokens[i].value;
				location.upload = true;
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}
			else if (tokens[i].type == word && tokens[i].value == "autoindex")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected auto index");
				}
				if (tokens[i].value == "on")
				{
					location.autoindex = true;
				}
				else if (tokens[i].value == "off")
				{
					location.autoindex = false;
				}
				else{					
					throw std::runtime_error("Invalid config: Expected auto index");
				}
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{					
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}
			else if (tokens[i].type == word && tokens[i].value == "allowed_methods")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected upload allowed methods");
				}
				location.allow_methods.clear();

				for ( int j = 1; j <= 3; j++)
				{
					if (tokens[i].type == word && (tokens[i].value == "GET" || tokens[i].value == "POST" || tokens[i].value == "DELETE"))
					{
						location.allow_methods.push_back(tokens[i].value);
						i++;
					}
					else if (tokens[i].type == semiColone && j != 1)
					{
						break;
					}
					else
					{
						// i think should return error code ? when method not found ?						
						throw std::runtime_error("Invalid config: unexpected upload method");
					}
				}
				if (tokens[i].type != semiColone)
				{
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}

			else if (tokens[i].type == word && tokens[i].value == "error_page")
			{
				// add this later: muti erros codes can share same error page
				// error_page 500 502 503 /50x.html;
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected error page");
				}
				unsigned int errorCode;
				std::stringstream ss(tokens[i].value);
				ss >> errorCode;
				if (ss.fail() || !ss.eof())
				{					
					throw std::runtime_error("Invalid config: Expected error code");
				}
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected error page path");
				}
				location.error_pages[errorCode] = tokens[i].value;
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}

			else if (tokens[i].type == word && tokens[i].value == "cgi_pass")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected upload path");
				}
				location.cgi_path = tokens[i].value;
				location.has_cgi = true;
				location.cgi_extension = location.path;
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}
			else if (tokens[i].type == word && tokens[i].value == "return")
			{
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected redirection status code");
				}
				unsigned int statusCode;
				std::stringstream ss(tokens[i].value);
				ss >> statusCode;
				if (ss.fail() || !ss.eof())
				{					
					throw std::runtime_error("Invalid config: Expected redirection status code");
				}
				if (statusCode != 301 && statusCode != 302)
				{					
					throw std::runtime_error("Invalid config: status code not allowed");
				}
				i++;
				if (i >= tokenSize || tokens[i].type != word)
				{					
					throw std::runtime_error("Invalid config: Expected redirection url");
				}
				if (location.has_redirect == true)
				{
					throw std::runtime_error("Invalid config: must be one redirection per location");
				}
				location.has_redirect = true;
				location.redirect_code = statusCode;
				location.redirect_url = tokens[i].value;
				i++;
				if (i >= tokenSize || tokens[i].type != semiColone)
				{
					throw std::runtime_error("Invalid config: Expected ;");
				}
				i++;
			}
			else
			{		
				std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
				throw std::runtime_error(str);
			}
		}
}

bool dupLocation(std::vector<LocationConfig> locations, std::string path)
{
	for (size_t i = 0; i < locations.size(); i++)
	{
		if(locations[i].path == path)
		{
			return true;
		}
	}
	return false;
}

void parseServer(std::vector<Token> &tokens, size_t &i, ServerConfig &server)
{
	size_t tokenSize = tokens.size();
	if (i >= tokenSize)
	{		
		throw std::runtime_error("Invalid config: expected }");
	}

	while ( i < tokenSize)
	{
		if (tokens[i].type == closeBrace)
		{
			i++;
			return;
		}
		if (tokens[i].type == word && tokens[i].value == "listen")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected port");
			}
			std::string port;
			size_t pos = 0;
			pos = tokens[i].value.find(':');
			if (pos != std::string::npos)
			{
				server.host = tokens[i].value.substr(0, pos);
				if (server.host.empty())
				{					
					throw std::runtime_error("Invalid config: invalid host");
				}
				port = tokens[i].value.substr(pos + 1);
				if (server.host.empty() || !isNumber(port) || !isValidPort(port))
				{					
					throw std::runtime_error("Invalid config: invalid port");
				}
				server.ports.clear();
				server.ports.push_back(port);
			}
			else
			{
				if (tokens[i].value.empty() || !isNumber(tokens[i].value) || !isValidPort(tokens[i].value))
				{					
					throw std::runtime_error("Invalid config: invalid port");
				}
				server.ports.clear();
				server.ports.push_back(tokens[i].value);
			}
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "server_name")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected server name");
			}
			server.server_name = tokens[i].value;
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "root")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected root");
			}
			server.root = tokens[i].value;
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "index")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected index");
			}
			server.index = tokens[i].value;
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "error_page")
		{
			// add this later: muti erros codes can share same error page
			// error_page 500 502 503 /50x.html;
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected error page");
			}
			unsigned int errorCode;
			std::stringstream ss(tokens[i].value);
			ss >> errorCode;
			if (ss.fail() || !ss.eof())
			{				
				throw std::runtime_error("Invalid config:Expected error code ");
			}
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected error page path");
			}
			server.error_pages[errorCode] = tokens[i].value;
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "client_max_body_size")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: Expected client max body size");
			}
			unsigned long value;
			std::string remaining;
			std::stringstream ss(tokens[i].value);
			ss >> value;
			if (ss.fail())
			{				
				throw std::runtime_error("Invalid config: client max body size not valid");
			}
			if (!ss.eof())
			{
				ss >> remaining;
				if (remaining == "K" || remaining == "KB")
					value *= 1024;
				else if (remaining == "M" || remaining == "MB")
					value *= 1024 * 1024;
				else if (remaining == "G" || remaining == "GB")
					value *= 1024 * 1024 *1024;
				else {					
					throw std::runtime_error("Invalid config: client_max_body_size not valid");
				}
			}
			server.client_max_body_size = value;
			i++;
			if (i >= tokenSize || tokens[i].type != semiColone)
			{
				throw std::runtime_error("Invalid config: Expected ;");
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "location")
		{
			i++;
			if (i >= tokenSize || tokens[i].type != word)
			{				
				throw std::runtime_error("Invalid config: unexpected location path");
			}
			LocationConfig location;
			location.autoindex = false;
			location.upload = false;
			location.has_redirect = false;
			location.has_cgi = false;

			if (dupLocation(server.locations, tokens[i].value))
			{
				std::string dupPath = "Invalid config: duplicate location " + tokens[i].value;
				throw std::runtime_error(dupPath);
			}

			location.path = tokens[i].value;
			// std::cout << "location path: " << location.path << std::endl;
			i++;
			if (i >= tokenSize || tokens[i].type != openBrace)
			{				
				throw std::runtime_error("Invalid config: expected { after location");
			}
			i++;
			parseLocation(tokens, i, location);
			server.locations.push_back(location);
		}
		else
		{
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
}

void Config::parse(const std::string &filename) {

	std::vector<Token> tokens = tokenize(filename);
	// for (size_t i = 0; i < tokens.size(); i++)
	// {
	// 	std::cout << tokens[i].value << " | " << tokens[i].type << std::endl;
	// }
	// return;
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
			if (_servers.size() != 0 && server.ports == _servers.back().ports)
			{
				if (server.server_name == _servers.back().server_name || server.host == _servers.back().host)
				{
					throw std::runtime_error("Invalid config: Invalid server config");
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
	out << "      has_cgi: " << loc.has_cgi<< std::endl;
	out << "      cgi_pass: " << loc.cgi_path << std::endl;
	out << "      cgi_extension: " << loc.cgi_extension  << std::endl;
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
	if (!server.error_page.empty())
		out << "    Error Page: " << server.error_page << std::endl;
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
