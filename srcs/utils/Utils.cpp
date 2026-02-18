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

std::string ipv6Tostr(const unsigned char ip[16]) {
	std::stringstream ss;
	ss << std::hex;
	for (int i = 0; i < 16; i += 2) {
		uint16_t group = (static_cast<uint16_t>(ip[i]) << 8) | ip[i + 1];
		ss << group;
		if (i < 14)
			ss << ":";
	}
	return ss.str();
}
