#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer(const Config& conf) : _epollFd(-1), _config(conf) {
	std::memset(_events, 0, sizeof(_events));
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		THROW_ERROR("epoll_create", "failed to create epoll instance");
	const std::vector<ServerConfig>& servers = _config.getServers();
	for (std::size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig& srv = servers[i];
		for (std::size_t j = 0; j < srv.ports.size(); ++j) {
			Socket sock(srv.host, srv.ports[j], &srv);
			sock.createAndBind();
			sock.setNonBlocking();
			sock.startListening(128);
			EPOLL_EVENT(ev);
			ev.events  = EPOLLIN;
			ev.data.fd = sock.getFd();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
				THROW_ERROR("epoll_ctl", "EPOLL_CTL_ADD fd=" + toString(ev.data.fd));
			_serverSockets[sock.getFd()] = sock;
			// LOG("Listening             | " << sock);
		}
	}
	// std::cout << conf;
}

WebServer::~WebServer() {
	for (Socket::It it = _serverSockets.begin(); it != _serverSockets.end(); ++it)
		it->second.close();
	for (Client::It it = _clients.begin(); it != _clients.end(); ++it)
		it->second.getSocket().close();
	if (_epollFd != -1)
		::close(_epollFd);
}

void WebServer::removeClient(int fd) {
	Client::It it = _clients.find(fd);
	if (it == _clients.end())
		return;
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	it->second.getSocket().close();
	_clients.erase(it);
}

void WebServer::acceptClient(Socket& serverSock) {
	Socket newSock;
	try {
		newSock = serverSock.accept();
		newSock.setNonBlocking();
	} catch (const std::exception& e) {
		newSock.close();
		return;
	}
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN;
	ev.data.fd = newSock.getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		PRINT_WARNING("epoll_ctl", "EPOLL_CTL_ADD fd=" + toString(ev.data.fd));
		newSock.close();
		return;
	}
	_clients[ev.data.fd] = Client(newSock, serverSock.getServerConf(), &_sessions);
	// LOG("New client            | " << newSock);
}

bool WebServer::handleRead(int fd) {
	Client& client = _clients[fd];
	if (!client.readData())
		return false;
	if (!client.isRequestComplete())
		return true;
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN | EPOLLOUT;
	ev.data.fd = fd;
	epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev);
	return true;
}

bool WebServer::handleWrite(int fd) {
	Client& client = _clients[fd];
	if (!client.sendData())
		return false;
	return true;
}

void WebServer::run() {
	std::cout << _config;
	while (running) {
		int numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, 100);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			THROW_ERROR("epoll_wait", "failed to wait for events");
		}
		for (int i = 0; i < numEvents; ++i) {
			int			fd	  = _events[i].data.fd;
			bool		keep  = true;
			const char* event = NULL;
			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
				event = GRN "client connected" RST;
			} else {
				if (_events[i].events & EPOLLIN) {
					keep = handleRead(fd);
					if (keep)
						event = CYN "request received" RST;
				}
				if (keep && (_events[i].events & EPOLLOUT)) {
					keep = handleWrite(fd);
					if (keep)
						event = YEL "response sent" RST;
				}
				if (!keep) {
					event = GRY "client disconnected" RST;
					removeClient(fd);
				}
			}
			if (event) {
				std::cout << GRY "── " RST << event << GRY " ──" RST "\n";
				std::cout << *this;
			}
		}
	}
}

const Socket::Map& WebServer::getServerSockets() const {
	return _serverSockets;
}
const Client::Map& WebServer::getClients() const {
	return _clients;
}
const SessionInfo::Map& WebServer::getSessions() const {
	return _sessions;
}
const Config& WebServer::getConfig() const {
	return _config;
}

std::ostream& operator<<(std::ostream& out, const WebServer& ws) {
	const Socket::Map& socks   = ws.getServerSockets();
	const Client::Map& clients = ws.getClients();

	out << "\n" GRY "┌─ WebServer ──────────────────────────────────┐" RST "\n";

	// --- Server Sockets ---
	out << GRY "│" RST " " WHT "Sockets" RST << GRY " [" RST << socks.size() << GRY "]" RST "\n";

	if (socks.empty()) {
		out << GRY "│  (none)\n" RST;
	} else {
		for (Socket::Map::const_iterator it = socks.begin(); it != socks.end(); ++it) {
			const Socket& s = it->second;
			out << GRY "│  " RST;
			out << GRY "fd=" RST << YEL << s.getFd() << RST;
			out << "  " << WHT << s.getIp() << GRY ":" RST << s.getPort() << RST;
			if (s.isBound())
				out << "  " << GRN "bound" RST;
			if (s.isListening())
				out << "  " << GRN "listening" RST;
			out << "\n";
		}
	}

	out << GRY "│" RST "\n";

	// --- Clients ---
	out << GRY "│" RST " " WHT "Clients" RST << GRY " [" RST << clients.size() << GRY "]" RST "\n";

	if (clients.empty()) {
		out << GRY "│  (none)\n" RST;
	} else {
		for (Client::Map::const_iterator it = clients.begin(); it != clients.end(); ++it) {
			const Socket&	s	= const_cast<Client&>(it->second).getSocket();
			const Request&	req = it->second.getRequest();
			const Response& res = it->second.getResponse();

			out << GRY "│  " RST;
			out << GRY "fd=" RST << YEL << s.getFd() << RST;
			out << "  " << WHT << s.getIp() << GRY ":" RST << s.getPort() << RST;
			if (!req.getMethod().empty())
				out << "  " << CYN << req.getMethod() << " " << req.getUri() << RST;
			out << "  " << (res.getStatusCode() ? GRN : GRY);
			if (res.getStatusCode())
				out << res.getStatusCode() << " " << res.getStatusMessage();
			else
				out << "no response";
			out << RST << "\n";
		}
	}

	out << GRY "└──────────────────────────────────────────────┘" RST "\n";
	return out;
}
