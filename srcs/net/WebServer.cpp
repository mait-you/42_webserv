#include "../../includes/net/WebServer.hpp"

#include "../../includes/utils/Logger.hpp"
#include "../../includes/utils/Utils.hpp"

bool WebServer::running = true;

WebServer::WebServer(const Config& conf) : _epollFd(-1), _config(conf) {
	std::memset(_events, 0, sizeof(_events));
	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1)
		ERROR_LOG("epoll_create", "failed to create epoll instance");
	const std::vector<ServerConfig>& servers = _config.getServers();
	for (std::size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig& srv = servers[i];
		for (std::size_t j = 0; j < srv.listens.size(); ++j) {
			Socket sock(srv.listens[j].host, srv.listens[j].port, &srv);
			sock.setup();
			EPOLL_EVENT(ev);
			ev.events  = EPOLLIN;
			ev.data.fd = sock.getFd();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
				ERROR_LOG("epoll_ctl", "EPOLL_CTL_ADD fd=" + toString(ev.data.fd));
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
const std::map<std::string, std::string>& WebServer::getSessions() const {
	return _sessions;
}
const Config& WebServer::getConfig() const {
	return _config;
}

void WebServer::removeClient(Client& client) {
	int fd = client.getSocket().getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		WARNING_LOG("WebServer::removeClient", "epoll_ctl DEL failed");
	client.getSocket().close();
	_clients.erase(fd);
}

void WebServer::acceptClient(Socket& serverSock) {
	Socket newSock;
	try {
		newSock = serverSock.accept();
	} catch (const std::exception& e) {
		newSock.close();
		return;
	}
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN;
	ev.data.fd = newSock.getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		WARNING_LOG("epoll_ctl", "EPOLL_CTL_ADD fd=" + toString(ev.data.fd));
		newSock.close();
		return;
	}
	_clients[ev.data.fd] = Client(newSock, serverSock, &_sessions);
}

bool WebServer::handleRequest(Client& client) {
	if (!client.recvData())
		return false;
	return client.parseRequest();
}

bool WebServer::handleResponse(Client& client) {
	if (!client.buildResponse())
		return true;
	return client.sendData();
}

void WebServer::run() {
	logServerStart(*this);

	while (running) {
		int n = epoll_wait(_epollFd, _events, MAX_EVENTS, 100);
		if (n == -1) {
			if (errno == EINTR)
				continue;
			ERROR_LOG("epoll_wait", "failed to wait for events");
		}

		for (int i = 0; i < n; ++i) {
			int fd = _events[i].data.fd;

			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
				logServerEvent(*this, GRN "Client connected" RST);
				continue;
			}

			Client::It it = _clients.find(fd);
			if (it == _clients.end())
				continue;
			Client& client = it->second;
			bool	keep   = true;

			if (_events[i].events & EPOLLIN) {
				keep = handleRequest(client);
				if (keep && client.getRequest().isComplete()) {
					EPOLL_EVENT(ev);
					ev.events  = EPOLLIN | EPOLLOUT;
					ev.data.fd = fd;
					epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev);
					logServerEvent(*this, CYN "Request received" RST);
				}
			}

			if (keep && (_events[i].events & EPOLLOUT)) {
				keep = handleResponse(client);
				if (client.getResponse().isComplete())
					logServerEvent(*this, YEL "Response sent" RST);
			}

			if (!keep) {
				removeClient(client);
				logServerEvent(*this, GRY "Client disconnected" RST);
			}
		}
	}
	logServerEvent(*this, GRY "Server shutdown" RST);
	std::cout << GRY "\r└─── ─ ─ ─ " RST "\n";
}
