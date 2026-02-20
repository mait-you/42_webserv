#include "../../includes/WebServer.hpp"
#include "../../includes/head.hpp"

void setupSignals() {
	signal(SIGINT, WebServer::stop);
}

bool isNumber(std::string str)
{
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); i++)
	{
		if (!isdigit(str[i]))
			return false;
	}
	return true;
}

bool isValidPort(std::string str)
{
	int port = -1;
	std::stringstream ss(str);
	ss >> port;
	if (port >= 1 && port <= 65535)
		return true;
	return false;
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

std::string getLine(const std::string &raw, size_t &pos) {
	std::string line;
	std::size_t end = raw.find("\r\n", pos);
	if (end == std::string::npos)
		end = raw.find("\n", pos);
	if (end == std::string::npos) {
		line = raw.substr(pos);
		pos	 = raw.size();
		return line;
	}
	line = raw.substr(pos, end - pos);
	pos	 = end + (raw[end] == '\r' ? 2 : 1);
	return line;
}

std::string trim(const std::string &s) {
	std::size_t start = s.find_first_not_of(" \t");
	if (start == std::string::npos)
		return "";
	std::size_t end = s.find_last_not_of(" \t");
	return s.substr(start, end - start + 1);
}
