#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer() : _epollFd(-1) {
}

WebServer::~WebServer() {
	for (Socket::It it = _ServerSock.begin(); it != _ServerSock.end(); ++it)
		it->second.close();
	for (Client::It it = _clients.begin(); it != _clients.end(); ++it)
		it->second.getSocket().close();
	if (_epollFd != -1)
		close(_epollFd);
}

void WebServer::stop(int) {
	WebServer::running = false;
}

void WebServer::handleClientRead(int fd) {
	Client &client = _clients[fd];
	client.readData();
	if (!client.isRequestComplete())
		return;
	EPOLL_EVENT(ev);
	ev.data.fd = fd;
	ev.events  = EPOLLIN | EPOLLOUT;
	epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void WebServer::handleClientWrite(int fd) {
	Client &client = _clients[fd];
	client.sendData();
	if (!client.isResponseSent())
		return;
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	client.getSocket().close();
	_clients.erase(fd);
}

void WebServer::acceptNewClient(Socket &serverSock) {
	Socket newClient;
	try {
		newClient = serverSock.accept();
	} catch (const std::exception &e) {
		return;
	}
	newClient.setNonBlocking();
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN;
	ev.data.fd = newClient.getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, newClient.getFd(), &ev) == -1) {
		std::cerr << "epoll_ctl ADD failed: " << std::strerror(errno)
				  << std::endl;
		return;
	}
	_clients[newClient.getFd()] = Client(newClient);
	LOG("New client connected  | " << newClient);
}

void WebServer::init(const std::string &configFile) {
	_config.parse(configFile);
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
		throw std::runtime_error("epoll_create() failed");
	const std::vector<ServerConfig> &servers = _config.getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig &srv = servers[i];
		for (size_t j = 0; j < srv.ports.size(); ++j) {
			Socket sock(srv.host, srv.ports[j]);
			sock.createAndBind();
			sock.setNonBlocking();
			sock.listen(128);
			EPOLL_EVENT(ev);
			ev.events  = EPOLLIN;
			ev.data.fd = sock.getFd();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, sock.getFd(), &ev) == -1)
				throw std::runtime_error("epoll_ctl() failed: " +
										 std::string(std::strerror(errno)));
			_ServerSock[sock.getFd()] = sock;
			LOG("Server Socket         | " << sock);
		}
	}
}

void WebServer::run() {
	LOG("WebServer running...");
	while (WebServer::running) {
		int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait() failed");
		}
		for (int i = 0; i < numEvents; i++) {
			int fd = events[i].data.fd;
			if (_ServerSock.count(fd))
				acceptNewClient(_ServerSock[fd]);
			else {
				if (events[i].events & EPOLLIN)
					handleClientRead(fd);
				if (events[i].events & EPOLLOUT)
					handleClientWrite(fd);
			}
		}
		std::memset(&events, 0, sizeof(events));
	}
	LOG("WebServer shutting down...");
}
