#include "../../includes/Head.hpp"
#include "../../includes/net/WebServer.hpp"

static void appendErrno(std::ostream& out) {
	if (errno) {
		out << ": " << std::strerror(errno);
		errno = 0;
	}
}

void warnLog(const std::string& context, const std::string& detail) {
	std::cerr << YEL "[WARNING]" RST " " WHT << context << YEL " — " << detail << RST;
	appendErrno(std::cerr);
	std::cerr << "\n";
}

void throwError(const std::string& context, const std::string& detail) {
	std::ostringstream msg;
	msg << RED "[ERROR]" RST " " WHT << context << YEL " — " << detail << RST;
	appendErrno(msg);
	throw std::runtime_error(msg.str());
}

void logServerEvent(const WebServer&, const char* event) {
	std::cout << GRY "├── " RST << event << GRY "\n" RST;
}
