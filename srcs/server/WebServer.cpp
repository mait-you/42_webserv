#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer(const Config& conf) : _epollFd(-1), _config(conf) {
	std::memset(_events, 0, sizeof(_events));
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		THROW_ERROR("epoll_create", "failed to create epoll instance");
	if (fcntl(_epollFd, F_SETFD, FD_CLOEXEC) == -1)
		THROW_ERROR("fcntl", "failed to set FD_CLOEXEC");
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
		}
	}
}

WebServer::~WebServer() {
	for (Socket::It it = _serverSockets.begin(); it != _serverSockets.end(); ++it)
		it->second.close();
	for (Client::It it = _clients.begin(); it != _clients.end(); ++it)
		it->second.getSocket().close();
	if (_epollFd != -1)
		::close(_epollFd);
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

void WebServer::removeClient(Client& client) {
	int fd = client.getSocket().getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		PRINT_WARNING("WebServer::removeClient", "epoll_ctl DEL failed");
	client.getSocket().close();
	_clients.erase(fd);
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
}

bool WebServer::handleRequest(Client& client) {
	if (!client.recvData())
		return false;
	client.parseRequest();
	return true;
}

bool WebServer::handleResponse(Client& client) {
	if (!client.buildResponse())
		return true;		   // CGI still running, come back next iteration
	return client.sendData();  // false = done = close
}
static void printEvent(const char* event, const WebServer& ws) {
	std::cout << GRY "│ \n│── " RST << event << GRY " ──\n│ \n" RST;
	std::cout << ws;
}

void WebServer::run() {
	printPrefix();

	while (running) {
		int n = epoll_wait(_epollFd, _events, MAX_EVENTS, 100);
		if (n == -1) {
			if (errno == EINTR)
				continue;
			THROW_ERROR("epoll_wait", "failed to wait for events");
		}

		for (int i = 0; i < n; ++i) {
			int fd = _events[i].data.fd;

			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
				printEvent(GRN "Client connected" RST, *this);
				continue;
			}

			Client::It it = _clients.find(fd);
			if (it == _clients.end())
				continue;
			Client& client = it->second;
			bool keep = true;

			if (_events[i].events & EPOLLIN) {
				keep = handleRequest(client);
				if (keep && client.getRequest().isComplete()) {
					EPOLL_EVENT(ev);
					ev.events  = EPOLLIN | EPOLLOUT;
					ev.data.fd = fd;
					epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev);
					printEvent(CYN "Request received" RST, *this);
				}
			}

			if (keep && (_events[i].events & EPOLLOUT)) {
				keep = handleResponse(client);
				if (client.getResponse().isComplete())
					printEvent(YEL "Response sent" RST, *this);
			}

			if (!keep) {
				removeClient(client);
				printEvent(GRY "Client disconnected" RST, *this);
			}
		}
	}
	printEvent(GRY "Server shutdown" RST, *this);
	std::cout << GRY "\r└─── ─ ─ ─ " RST "\n";
}

void WebServer::printPrefix() {
	std::cout << _config;
	std::cout << "\n" GRY "┌─ WebServer" RST "\n";
	std::cout << GRY "│ " WHT "Server Sockets" GRY " [" RST << _serverSockets.size()
			  << GRY "]" RST "\n";
	if (_serverSockets.empty()) {
		std::cout << GRY "│   (none)\n" RST;
	} else {
		for (Socket::Map::const_iterator it = _serverSockets.begin(); it != _serverSockets.end();
			 ++it) {
			Socket::Map::const_iterator next = it;
			++next;
			bool isLast = (next == _serverSockets.end());
			std::cout << GRY "│   " RST << (isLast ? GRY "└─ " RST : GRY "├─ " RST) << it->second
					  << "\n";
		}
	}

	std::cout << GRY "│\n";
}
std::ostream& operator<<(std::ostream& out, const WebServer& ws) {
	const Client::Map& clients = ws.getClients();
	out << GRY "│ " WHT "Clients" GRY " [" RST << clients.size() << GRY "]" RST "\n";
	if (clients.empty()) {
		out << GRY "│   (none)\n" RST;
	} else {
		for (Client::Map::const_iterator it = clients.begin(); it != clients.end(); ++it) {
			Client::Map::const_iterator next = it;
			++next;
			bool isLast = (next == clients.end());
			printClient(out, it->second, isLast ? GRY "└─ " RST : GRY "├─ " RST,
						isLast ? GRY "    " RST : GRY "│   " RST);
		}
	}

	return out;
}
