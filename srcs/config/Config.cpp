#include "../../includes/Config.hpp"

Config::Config() {
}

Config::Config(const Config &) {
}

Config &Config::operator=(const Config &) {

	return *this;
}

Config::~Config() {
}

void Config::parse(const std::string &) {

	ServerConfig server;
	server.host = "localhost";
	server.ports.push_back("8080");
	server.server_name			= "localhost";
	server.root					= "./www";
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
