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

static void printEvent(const char* event, const WebServer& ws) {
	std::cout << GRY "│ \n│── " RST << event << GRY " ──\n│ \n" RST;
	std::cout << ws;
}

void WebServer::run() {
	std::cout << _config;
	std::cout << "\n" GRY "┌─ WebServer" RST "\n";
	std::cout << *this;

	while (running) {
		int numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, 100);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			THROW_ERROR("epoll_wait", "failed to wait for events");
		}
		for (int i = 0; i < numEvents; ++i) {
			int	 fd	  = _events[i].data.fd;
			bool keep = true;

			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
				printEvent(GRN "Client connected" RST, *this);
			} else {
				if (_events[i].events & EPOLLIN) {
					keep = handleRead(fd);
					if (_clients.count(fd) && _clients[fd].getRequest().isComplete())
						printEvent(CYN "Request received" RST, *this);
				}
				if (keep && (_events[i].events & EPOLLOUT)) {
					keep = handleWrite(fd);
					if (_clients.count(fd) && _clients[fd].getResponse().isComplete())
						printEvent(YEL "Response sent" RST, *this);
				}
				if (!keep) {
					removeClient(fd);
					printEvent(GRY "Client disconnected" RST, *this);
				}
			}
		}
	}
	printEvent(GRY "Server shutdown" RST, *this);
	std::cout << GRY "\r└─── ─ ─ ─ " RST "\n";
}

std::ostream& operator<<(std::ostream& out, const WebServer& ws) {
	const Socket::Map& socks   = ws.getServerSockets();
	const Client::Map& clients = ws.getClients();

	out << GRY "│ " WHT "Server Sockets" GRY " [" RST << socks.size() << GRY "]" RST "\n";
	if (socks.empty()) {
		out << GRY "│   (none)\n" RST;
	} else {
		for (Socket::Map::const_iterator it = socks.begin(); it != socks.end(); ++it) {
			Socket::Map::const_iterator next = it;
			++next;
			bool isLast = (next == socks.end());
			out << GRY "│   " RST << (isLast ? GRY "└─ " RST : GRY "├─ " RST) << it->second << "\n";
		}
	}

	out << GRY "│\n";

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
