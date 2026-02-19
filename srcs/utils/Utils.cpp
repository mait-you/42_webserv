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
