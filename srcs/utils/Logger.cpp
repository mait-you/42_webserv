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

static void logSocket(const Socket& s) {
	std::cout << GRY "[" CYN "fd=" RST YEL << s.getFd() << RST GRY "] " << WHT << s.getIp()
			  << GRY ":" WHT << s.getPort() << RST;
}

static void logRequest(const Request& req, bool hasRes) {
	const std::string p = hasRes ? GRY "│  │  " RST : GRY "│     " RST;

	const std::string& m = req.getMethod();
	const std::string& u = req.getUri();
	const std::string& v = req.getVersion();

	std::cout << p << GRY "├─ " WHT "method:  " RST << (m.empty() ? GRY "(none)" RST : m.c_str())
			  << "\n"
			  << p << GRY "├─ " WHT "uri:     " RST << (u.empty() ? GRY "(none)" RST : u.c_str())
			  << "\n"
			  << p << GRY "├─ " WHT "version: " RST << (v.empty() ? GRY "(none)" RST : v.c_str())
			  << "\n";

	const Request::HeaderMap& hdrs = req.getHeaders();
	std::cout << p << GRY "├─ " WHT "headers: " GRY "[" RST << hdrs.size() << GRY "]\n" RST;
	for (Request::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		std::cout << p << GRY "│   " RST << it->first << GRY ": " RST << it->second << "\n";

	const std::string& body = req.getBody();
	std::cout << p << GRY "├─ " WHT "body:    " RST;
	if (body.empty()) {
		std::cout << GRY "(empty)\n" RST;
	} else {
		std::cout << GRY "[" RST << body.size() << GRY " bytes] " RST << std::hex
				  << std::setfill('0');
		for (size_t i = 0; i < body.size() && i < 16; ++i)
			std::cout << std::setw(2)
					  << static_cast<unsigned int>(static_cast<unsigned char>(body[i])) << " ";
		if (body.size() > 16)
			std::cout << GRY "...";
		std::cout << std::dec << RST "\n";
	}

	std::cout << p << GRY "└─ " WHT "type:    " RST
			  << (req.hasCgi() ? CYN "dynamic" RST : GRY "static" RST) << "\n";
}

static void logResponse(const Response& res) {
	std::cout << GRY "│     ├─ " WHT "status:  " RST;
	if (res.getStatusCode() == 0)
		std::cout << GRY "(none)\n" RST;
	else
		std::cout << YEL << res.getStatusCode() << RST " " << res.getStatusMessage() << "\n";

	const Response::HeaderMap& hdrs = res.getHeaders();
	std::cout << GRY "│     ├─ " WHT "headers: " GRY "[" RST << hdrs.size() << GRY "]\n" RST;
	for (Response::ConstHeaderIt it = hdrs.begin(); it != hdrs.end(); ++it)
		std::cout << GRY "│     │   " RST << it->first << GRY ": " RST << it->second << "\n";

	std::cout << GRY "│     ├─ " WHT "body:    " RST;
	if (res.getBody().empty())
		std::cout << GRY "(empty)\n" RST;
	else
		std::cout << GRY "[" RST << res.getBody().size() << GRY " bytes]\n" RST;

	std::cout << GRY "│     └─ " WHT "cgi:     " RST
			  << (res.hasCgiRunning() ? CYN "running" RST : GRY "idle" RST) << "\n";
}

static void logClient(const Client& client) {
	bool hasReq = client.getRequest().isComplete();
	bool hasRes = client.getResponse().isComplete();

	std::cout << GRY "│  ├─ " RST;
	logSocket(client.getSocket());
	std::cout << "\n";

	if (hasReq) {
		std::cout << GRY "│  " RST << (hasRes ? GRY "├─ " : GRY "└─ ") << WHT "Request\n" RST;
		logRequest(client.getRequest(), hasRes);
	}
	if (hasRes) {
		std::cout << GRY "│  └─ " WHT "Response\n" RST;
		logResponse(client.getResponse());
	}
}

static void logClients(const WebServer& ws) {
	const Client::Map& clients = ws.getClients();
	std::cout << GRY "│ " WHT "Clients " GRY "[" RST << clients.size() << GRY "]\n" RST;
	if (clients.empty()) {
		std::cout << GRY "│   (none)\n" RST;
		return;
	}
	for (Client::Map::const_iterator it = clients.begin(); it != clients.end(); ++it)
		logClient(it->second);
}

static void logLocationConfig(const LocationConfig& loc, bool last) {
	const std::string branch = last ? "└─ " : "├─ ";
	const std::string pipe	 = last ? "   " : "│  ";

	std::cout << GRY "│    " RST << GRY << branch << RST << CYN << loc.path << RST "\n";

	std::cout << GRY "│    " RST << GRY << pipe << RST "methods: ";
	for (size_t i = 0; i < loc.allow_methods.size(); i++) {
		std::cout << GRN << loc.allow_methods[i] << RST;
		if (i + 1 < loc.allow_methods.size())
			std::cout << GRY ", " RST;
	}
	std::cout << "\n";

	if (!loc.root.empty())
		std::cout << GRY "│    " RST << GRY << pipe << RST "root:      " << WHT << loc.root
				  << RST "\n";
	if (!loc.index.empty())
		std::cout << GRY "│    " RST << GRY << pipe << RST "index:     " << WHT << loc.index
				  << RST "\n";
	if (loc.has_redirect)
		std::cout << GRY "│    " RST << GRY << pipe << RST "redirect:  " << YEL << loc.redirect_code
				  << " " << loc.redirect_url << RST "\n";

	std::cout << GRY "│    " RST << GRY << pipe
			  << RST "autoindex: " << (loc.autoindex ? GRN "on" : GRY "off") << RST "\n";
	std::cout << GRY "│    " RST << GRY << pipe
			  << RST "upload:    " << (loc.upload ? GRN "on" : GRY "off") << RST;
	if (loc.upload)
		std::cout << "  " << WHT << loc.upload_path << RST;
	std::cout << "\n";

	if (loc.has_cgi) {
		for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin();
			 it != loc.cgi.end(); ++it)
			std::cout << GRY "│    " RST << GRY << pipe << RST "cgi:       " << CYN << it->first
					  << RST " -> " << WHT << it->second << RST "\n";
	}
}

static void logServerConfig(const ServerConfig& server) {
	// std::cout << GRY "│  " RST << WHT << server.host << RST;

	std::cout << GRY ":" RST;
	for (size_t i = 0; i < server.listens.size(); i++) {
		std::cout << GRY "│  " RST << WHT << server.listens[i].host << RST;
		std::cout << YEL << server.listens[i].port << RST;
		if (i + 1 < server.listens.size())
			std::cout << GRY "," RST;
	}

	if (!server.server_name.empty())
		std::cout << "  " << GRY "[" RST << server.server_name << GRY "]" RST;

	std::cout << "\n";
	std::cout << GRY "│    " RST "root:      " << WHT << server.root << RST "\n";
	std::cout << GRY "│    " RST "index:     " << WHT << server.index << RST "\n";
	std::cout << GRY "│    " RST "max body:  " << YEL << server.client_max_body_size
			  << RST " bytes\n";

	if (!server.locations.empty()) {
		std::cout << GRY "│    " RST "locations:\n";
		for (size_t i = 0; i < server.locations.size(); i++)
			logLocationConfig(server.locations[i], i + 1 == server.locations.size());
	}
}

static void logConfig(const Config& config) {
	const std::vector<ServerConfig>& servers = config.getServers();

	std::cout << "\n" GRY "┌─ Config " RST << GRY "[" RST << servers.size() << GRY "]" RST "\n";
	for (size_t i = 0; i < servers.size(); i++) {
		std::cout << GRY "│ " RST << WHT "Server" RST << GRY " [" RST << (i + 1)
				  << GRY "]" RST "\n";
		logServerConfig(servers[i]);
	}
	std::cout << GRY "└─── ─ ─ ─ " RST "\n";
}

void logServerStart(const WebServer& ws) {
	logConfig(ws.getConfig());
	std::cout << GRY "┌─ WebServer\n"
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
		logSocket(it->second);
		std::cout << "\n";
	}
}

void logServerEvent(const WebServer& ws, const char* event) {
	std::cout << GRY "│\n├── " RST << event << GRY "\n│\n" RST;
	logClients(ws);
}
