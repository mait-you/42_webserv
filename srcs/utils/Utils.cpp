#include "../../includes/Head.hpp"
#include "../../includes/net/WebServer.hpp"

static void handler(int) {
	WebServer::running = false;
}

void setupSignals() {
	signal(SIGINT, handler);
	signal(SIGPIPE, SIG_IGN);
}

bool isNumber(const std::string& str) {
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); i++) {
		if (!std::isdigit(str[i]))
			return false;
	}
	return true;
}

bool isValidPort(const std::string& str) {
	int				  port = -1;
	std::stringstream ss(str);
	ss >> port;
	if (port >= 1024 && port <= 65535)
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



std::string toString(int val) {
	std::ostringstream ss;
	ss << val;
	return ss.str();
}

std::string htmlEscape(const std::string& s) {
	std::string out;

	for (std::string::size_type i = 0; i < s.size(); ++i) {
		switch (s[i]) {
			case '&':
				out += "&amp;";
				break;
			case '<':
				out += "&lt;";
				break;
			case '>':
				out += "&gt;";
				break;
			case '"':
				out += "&quot;";
				break;
			case '\'':
				out += "&#39;";
				break;
			default:
				out += s[i];
		}
	}
	return out;
}
