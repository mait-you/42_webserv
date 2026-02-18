#include "../../includes/WebServer.hpp"
#include "../../includes/head.hpp"

void setupSignals() {
	signal(SIGINT, WebServer::stop);
}

std::string portTostr(uint16_t port) {
	std::stringstream ss;
	ss << port;
	return ss.str();
}

std::string ipv4Tostr(uint32_t ip) {
	unsigned char	 *cIp = (unsigned char *) &ip;
	std::stringstream ss;
	ss << static_cast<int>(cIp[0]) << "." << static_cast<int>(cIp[1]) << "."
	   << static_cast<int>(cIp[2]) << "." << static_cast<int>(cIp[3]);
	return ss.str();
}

