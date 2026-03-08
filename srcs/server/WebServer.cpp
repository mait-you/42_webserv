#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer(const Config& conf) : _epollFd(-1), _config(conf) {
	std::memset(_events, 0, sizeof(_events));
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		throwError("epoll_create: ");
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
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, sock.getFd(), &ev) == -1)
				throwError("epoll_ctl: ");
			_serverSockets[sock.getFd()] = sock;
			LOG("Listening             | " << sock);
		}
	}
	// std::cout << conf << std::endl;
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
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, newSock.getFd(), &ev) == -1) {
		std::cerr << "epoll_ctl ADD: " << std::strerror(errno) << std::endl;
		newSock.close();
		return;
	}
	_clients[newSock.getFd()] = Client(newSock, serverSock.getServerConf());
	LOG("New client            | " << newSock);
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
	LOG("WebServer running...");
	while (running) {
		int numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			throwError("epoll_wait: ");
		}
		for (int i = 0; i < numEvents; ++i) {
			int fd = _events[i].data.fd;
			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
			} else {
				bool keep = true;
				if (_events[i].events & EPOLLIN)
					keep = handleRead(fd);
				if (keep && (_events[i].events & EPOLLOUT))
					keep = handleWrite(fd);
				if (!keep)
					removeClient(fd);
			}
		}
	}
	LOG("WebServer shutting down...");
}
