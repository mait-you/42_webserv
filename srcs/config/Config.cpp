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

void Config::parse(const std::string &) {
	LOG_DEBUGG("Config", "Parsing configuration file: ");

	ServerConfig server;
	server.host = "localhost";
	server.ports.push_back("8080");
	server.root					= "./www";
	server.index				= "index.html";
	server.error_page			= "./www/error_pages/404.html";
	server.client_max_body_size = 1048576; // 1 MB
	LocationConfig location;
	location.path = "/";
	location.allow_methods.push_back("GET");
	location.allow_methods.push_back("POST");
	location.allow_methods.push_back("DELETE");
	location.autoindex	 = false;
	location.upload		 = false;
	location.upload_path = "";
	server.locations.push_back(location);

	_servers.push_back(server);
}

const std::vector<ServerConfig> &Config::getServers() const {
	return _servers;
}
