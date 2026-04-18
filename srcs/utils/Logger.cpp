#include "../../includes/Head.hpp"
#include "../../includes/net/WebServer.hpp"

static void appendErrno(std::ostream& out) {
	if (errno) {
		out << ": " << std::strerror(errno);
		errno = 0;
	}
}

void warnLog(const std::string& file, int line, const std::string& context,
			 const std::string& detail) {
	std::cerr << YEL "[WARNING]" RST " " WHT << context << YEL " — " << detail << RST;
	appendErrno(std::cerr);
	std::cerr << GRY " (" << file << ":" << line << ")" RST "\n";
}

void errorLog(const std::string& file, int line, const std::string& context,
			  const std::string& detail) {
	std::ostringstream msg;
	msg << RED "[ERROR]" RST " " WHT << context << YEL " — " << detail << RST;
	appendErrno(msg);
	msg << GRY " (" << file << ":" << line << ")" RST;
	throw std::runtime_error(msg.str());
}

static void printSocket(std::ostream& out, const Socket& s) {
	out << GRY "[" CYN "fd=" RST YEL << s.getFd() << RST GRY "] " << WHT << s.getIp() << GRY ":" WHT
		<< s.getPort() << RST;
}

static void printRequest(std::ostream& out, const Request& req, bool hasRes) {
	const std::string p = hasRes ? GRY "│  │  " RST : GRY "│     " RST;

	const std::string& m = req.getMethod();
	const std::string& u = req.getUri();
	const std::string& v = req.getVersion();

	out << p << GRY "├─ " WHT "Method  " RST << (m.empty() ? GRY "(none)" RST : m.c_str()) << "\n"
		<< p << GRY "├─ " WHT "URI     " RST << (u.empty() ? GRY "(none)" RST : u.c_str()) << "\n"
		<< p << GRY "├─ " WHT "Version " RST << (v.empty() ? GRY "(none)" RST : v.c_str()) << "\n";

	const Request::HeaderMap& hdrs = req.getHeaders();
	out << p << GRY "├─ " WHT "Headers " GRY "[" RST << hdrs.size() << GRY "]\n" RST;
	for (Request::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		out << p << GRY "│   " RST << it->first << GRY ": " RST << it->second << "\n";

	const std::string& body = req.getBody();
	out << p << GRY "├─ " WHT "Body    " RST;
	if (body.empty()) {
		out << GRY "(empty)\n" RST;
	} else {
		out << GRY "[" RST << body.size() << GRY " bytes] " RST << std::hex << std::setfill('0');
		for (size_t i = 0; i < body.size() && i < 16; ++i)
			out << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(body[i]))
				<< " ";
		if (body.size() > 16)
			out << GRY "...";
		out << std::dec << RST "\n";
	}

	out << p << GRY "└─ " WHT "Type    " RST
		<< (req.hasCgi() ? CYN "dynamic" RST : GRY "static" RST) << "\n";
}

static void printResponse(std::ostream& out, const Response& res) {
	out << GRY "│     ├─ " WHT "Status  " RST;
	if (res.getStatusCode() == 0)
		out << GRY "(none)\n" RST;
	else
		out << YEL << res.getStatusCode() << RST " " << res.getStatusMessage() << "\n";

	const Response::HeaderMap& hdrs = res.getHeaders();
	out << GRY "│     ├─ " WHT "Headers " GRY "[" RST << hdrs.size() << GRY "]\n" RST;
	for (Response::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		out << GRY "│     │   " RST << it->first << GRY ": " RST << it->second << "\n";

	out << GRY "│     ├─ " WHT "Body    " RST;
	if (res.getBody().empty())
		out << GRY "(empty)\n" RST;
	else
		out << GRY "[" RST << res.getBody().size() << GRY " bytes]\n" RST;

	out << GRY "│     └─ " WHT "CGI     " RST
		<< (res.hasCgiRunning() ? CYN "running" RST : GRY "idle" RST) << "\n";
}

static void printClient(std::ostream& out, const Client& client) {
	bool hasReq = client.getRequest().isComplete();
	bool hasRes = client.getResponse().isComplete();

	out << GRY "│  ├─ " RST;
	printSocket(out, client.getSocket());
	out <<  RST "\n";

	if (hasReq) {
		out << GRY "│  " RST << (hasRes ? GRY "├─ " : GRY "└─ ") << WHT "Request\n" RST;
		printRequest(out, client.getRequest(), hasRes);
	}
	if (hasRes) {
		out << GRY "│  └─ " WHT "Response\n" RST;
		printResponse(out, client.getResponse());
	}
}

static void printClients(std::ostream& out, const WebServer& ws) {
	const Client::Map& clients = ws.getClients();
	out << GRY "│ " WHT "Clients " GRY "[" RST << clients.size() << GRY "]\n" RST;
	if (clients.empty()) {
		out << GRY "│   (none)\n" RST;
		return;
	}
	for (Client::Map::const_iterator it = clients.begin(); it != clients.end(); ++it)
		printClient(out, it->second);
}

void printPrefix(const WebServer& ws) {
	std::cout << ws.getConfig() << "\n"
			  << GRY "┌─ WebServer\n"
			  << "│ " WHT "Server Sockets " GRY "[" RST << ws.getServerSockets().size()
			  << GRY "]\n" RST;

	const Socket::Map& sockets = ws.getServerSockets();
	if (sockets.empty()) {
		std::cout << GRY "│   (none)\n" RST;
		return;
	}
	for (Socket::Map::const_iterator it = sockets.begin(); it != sockets.end(); ++it) {
		Socket::Map::const_iterator next = it;
		++next;
		std::cout << GRY "│   " RST << (next == sockets.end() ? GRY "└─ " RST : GRY "├─ " RST);
		printSocket(std::cout, it->second);
		std::cout << "\n";
	}
	std::cout << GRY "│\n" RST;
}

void printEvent(const WebServer& ws, const char* event) {
	std::cout << GRY "│\n├── " RST << event << GRY "\n│\n" RST;
	printClients(std::cout, ws);
}
