#include "../../includes/Config.hpp"

Config::Config() {
	LOG_DEBUGG("Config", "Default constructor called");
}

Config::Config(const Config &) {
	LOG_DEBUGG("Config", "Copy constructor called");
}

Config &Config::operator=(const Config &) {
	LOG_DEBUGG("Config", "Copy assignment constructor called");
	return *this;
}

Config::~Config() {
	LOG_DEBUGG("Config", "Destructor constructor called");
}


std::vector<Token> tokenize(const std::string &filename)
{
	char c;
	struct Token token;
	std::vector<Token> tokens;

	std::ifstream configFile(filename.c_str());
	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0)
	{
		std::cerr << "Error: cannot open config file" << std::endl;
		return tokens;
	}
	while (configFile.get(c))
	{
		if (isspace(c))
			continue;
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
}

void parseLocation(std::vector<Token> &tokens, int &i, LocationConfig &location)
{
		while (i < tokens.size())
		{
			if (tokens[i].type == closeBrace)
			{
				i++;
				return;
			}
			if (tokens[i].type == word && tokens[i].value == "autoindex")
			{
				i++;
				if (tokens[i].type != word)
				{
					std::cerr << "Expected auto index" << std::endl;
					return;
				}
				location.upload_path = tokens[i].value;
				i++;
				if (tokens[i].type != semiColone)
				{
					std::cerr << "Expected ;" << std::endl;
					return;
				}
				i++;
			}
			if (tokens[i].type == word && tokens[i].value == "upload_path")
			{
				i++;
				if (tokens[i].type != word)
				{
					std::cerr << "Expected upload path" << std::endl;
					return;
				}
				location.upload_path = tokens[i].value;
				i++;
				if (tokens[i].type != semiColone)
				{
					std::cerr << "Expected ;" << std::endl;
					return;
				}
				i++;
			}
			if (tokens[i].type == word && tokens[i].value == "allowed_methods")
			{
				i++;
				if (tokens[i].type != word)
				{
					std::cerr << "Expected upload allowed methods" << std::endl;
					return;
				}
				int j = 1;
				while (j <= 3)
				{
					if (tokens[i].type == word)
					{
						location.allow_methods.push_back(tokens[i].value);
						i++;
					}
					else if (tokens[i].type == semiColone)
					{
						break;
					}
					else
					{
						std::cerr << "unexpected upload method" << std::endl;
						return;
					}
				}
				if (tokens[i].type != semiColone)
				{
					std::cerr << "Expected ;" << std::endl;
					return;
				}
				i++;
			}

			if (tokens[i].type == word && tokens[i].value == "cgi_pass")
			{
				i++;
				if (tokens[i].type != word)
				{
					std::cerr << "Expected upload path" << std::endl;
					return;
				}
				location.cgi_path = tokens[i].value;
				location.has_cgi = true;
				location.cgi_extension = location.path;
				i++;
				if (tokens[i].type != semiColone)
				{
					std::cerr << "Expected ;" << std::endl;
					return;
				}
				i++;
			}
		}
}

void parseServer(std::vector<Token> &tokens, int &i, ServerConfig &server)
{
	server.client_max_body_size = 1024 * 1024;
	while ( i < tokens.size())
	{
		if (tokens[i].type == closeBrace)
		{
			i++;
			return;
		}
		if (tokens[i].type == word && tokens[i].value == "listen")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected port" << std::endl;
				return;
			}
			server.ports.push_back(tokens[i].value);
			i++;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "server_name")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected server name" << std::endl;
				return;
			}
			server.server_name = tokens[i].value;
			i++;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "root")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected root" << std::endl;
				return;
			}
			server.root = tokens[i].value;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "index")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected index" << std::endl;
				return;
			}
			server.index = tokens[i].value;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "error_page")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected error page" << std::endl;
				return;
			}
			server.error_page = tokens[i].value;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "client_max_body_size")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "Expected client max body size" << std::endl;
				return;
			}
			unsigned long value;
			std::string remaining;
			std::stringstream ss(tokens[i].value);
			ss >> value;
			if (ss.fail())
			{
				std::cerr << "client max body size not valid" << std::endl;
				return;
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
					std::cerr << "client_max_body_size not valid" << std::endl;
					return;
				}
			}
			server.client_max_body_size = value;
			if (tokens[i].type != semiColone)
			{
				std::cerr << "Expected ;" << std::endl;
				return;
			}
			i++;
		}
		else if (tokens[i].type == word && tokens[i].value == "location")
		{
			i++;
			if (tokens[i].type != word)
			{
				std::cerr << "unexpected location path" << std::endl;
				return;
			}
			LocationConfig location;
			location.autoindex = false;
			location.upload = false;
			location.has_redirect = false;
			location.has_cgi = false;

			location.path = tokens[i].value;
			if (location.path == "/upload")
				location.upload = true;
			i++;
			if (tokens[i].type != openBrace)
			{
				std::cerr << "expected { after location" << std::endl;
				return;
			}
			i++;
			parseLocation(tokens, i, location);
			server.locations.push_back(location);
			// i++;
		}
		else
		{
			std::cerr << "unexpected token: " << tokens[i].value << std::endl;
			return;
		}
	}
}

void Config::parse(const std::string &filename) {
	// File → Lexer → Vector<Token> → Parser → Config Structure
	// file reading + tokenizing + parsing logic.
	// 1- read file	
	// 2-Tokenize
	// 3-Validate structure
	// 4-Store in structs

	std::vector<Token> tokens = tokenize(filename);

	int i = 0;

	while ( i < tokens.size())
	{
		if (tokens[i].type == word && tokens[i].value == "server")
		{
			i++;
			if (tokens[i].type != openBrace)
			{
				std::cerr << "expected { after server" << std::endl;
				return;
			}
			i++;
			ServerConfig server;
			parseServer(tokens, i, server);
			_servers.push_back(server);
		}
		else
		{
			std::cerr << "unexpected token: " << tokens[i].value << std::endl;
		}
		i++;
	}
	

	ServerConfig server;
	server.host = "127.0.0.1";
	server.ports.push_back("8080");
	server.server_name			= "localhost";
	server.root					= "/var/www";
	server.index				= "index.html";
	server.error_page			= "";
	server.client_max_body_size = 1000000;

	LocationConfig loc;
	loc.path = "/";
	loc.allow_methods.push_back("GET");
	loc.allow_methods.push_back("POST");
	loc.autoindex	  = true;
	loc.upload		  = false;
	loc.upload_path	  = "";
	loc.root		  = "";
	loc.index		  = "";
	loc.has_redirect  = false;
	loc.redirect_url  = "";
	loc.redirect_code = 0;

	server.locations.push_back(loc);
	_servers.push_back(server);
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
