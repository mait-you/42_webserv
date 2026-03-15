#include "../../includes/Head.hpp"
#include "../../includes/WebServer.hpp"

static void handler(int) {
	WebServer::running = false;
}

void setupSignals() {
	signal(SIGINT, handler);
}

bool isNumber(const std::string& str) {
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); i++) {
		if (!isdigit(str[i]))
			return false;
	}
	return true;
}

bool isValidPort(const std::string& str) {
	int				  port = -1;
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
	unsigned char*	  cIp = (unsigned char*) &ip;
	std::stringstream ss;
	ss << static_cast<int>(cIp[0]) << "." << static_cast<int>(cIp[1]) << "."
	   << static_cast<int>(cIp[2]) << "." << static_cast<int>(cIp[3]);
	return ss.str();
}

std::string toLower(const std::string& s) {
	std::string r = s;
	for (std::size_t i = 0; i < r.size(); ++i)
		r[i] = static_cast<char>(std::tolower(r[i]));
	return r;
}

std::string trimStr(const std::string& s) {
	std::size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	std::size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

bool getLine(const std::string& buf, std::size_t& pos, std::string& line) {
	std::size_t end = buf.find("\r\n", pos);
	if (end == std::string::npos)
		return false;
	line = buf.substr(pos, end - pos);
	pos	 = end + 2;
	return true;
}

std::string cleanUri(const std::string& uri) {
	if (uri.empty())
		return "";
	std::string str = uri;
	std::size_t qpos = str.find('?');
	if (qpos != std::string::npos)
		str = str.substr(0, qpos);

	std::string				 segment;
	std::vector<std::string> cleanPath;
	std::stringstream		 ss(str);

	while (std::getline(ss, segment, '/')) {
		if (segment.empty() || segment == ".")
			continue;
		if (segment == "..") {
			if (!cleanPath.empty())
				cleanPath.pop_back();
		} else {
			cleanPath.push_back(segment);
		}
	}

	std::string buffer;
	for (size_t i = 0; i < cleanPath.size(); i++) {
		buffer += "/";
		buffer += cleanPath[i];
	}
	if (cleanPath.empty() || uri[uri.size() - 1] == '/')
		buffer += "/";

	return buffer;
}

std::string getExtension(const std::string& fullPath) {
	std::string name	  = fullPath;
	std::size_t lastSlash = name.find_last_of('/');
	if (lastSlash != std::string::npos)
		name = name.substr(lastSlash + 1);
	std::size_t pos = name.find_last_of('.');
	if (pos != std::string::npos)
		return name.substr(pos + 1);
	return "";
}
