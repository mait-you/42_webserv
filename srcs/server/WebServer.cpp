#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer() : _epollFd(-1) {
}

WebServer::~WebServer() {
	for (size_t i = 0; i < _ServerSockets.size(); ++i)
		_ServerSockets[i].close();
	for (Client::ClientIterator it = _clients.begin(); it != _clients.end(); ++it)
		it->second.getSocket().close();
	if (_epollFd != -1)
		close(_epollFd);
}

void WebServer::stop(int) {
	WebServer::running = false;
}

void WebServer::init(const std::string &configFile) {
	_config.parse(configFile);
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		throw std::runtime_error("epoll_create1() failed");
	const std::vector<ServerConfig> &webServers = _config.getServers();
	for (size_t i = 0; i < webServers.size(); ++i) {
		const ServerConfig &server = webServers[i];
		for (size_t j = 0; j < server.ports.size(); ++j) {
			Socket socket(server.host, server.ports[j]);
			socket.createAndBind();
			socket.setNonBlocking();
			socket.listen(128);
			_ServerSockets.push_back(socket);
			LOG("Server Socket         | " << socket);
		}
	}
	for (size_t i = 0; i < _ServerSockets.size(); ++i) {
		EPOLL_EVENT(ev);
		ev.events  = EPOLLIN;
		ev.data.fd = _ServerSockets[i].getFd();

		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
			throw std::runtime_error(
				"epoll_ctl() failed for listening socket " +
				std::string(std::strerror(errno)));
	}
}

void WebServer::acceptNewClient(Socket &socket) {
	Socket nweClient;
	try {
		nweClient = socket.accept();
	} catch (const std::exception &e) {
		return;
	}
	Client client(nweClient);
	client.getSocket().setNonBlocking();
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN;
	ev.data.fd = nweClient.getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, nweClient.getFd(), &ev) == -1) {
		std::cout << "epoll_ctl ADD failed: " << std::strerror(errno)
				  << std::endl;
		return;
	}
	_clients[nweClient.getFd()] = client;
	LOG("New client connected  | " << client.getSocket());
}

void WebServer::handleClientRead(Socket &socket) {
	Client::ClientIterator it = _clients.find(socket.getFd());
	if (it == _clients.end())
		return;
	LOG("Request received      | " << it->second);
	it->second.readData();
	EPOLL_EVENT(ev);
	ev.events  = EPOLLIN | EPOLLOUT;
	ev.data.fd = socket.getFd();
	epoll_ctl(_epollFd, EPOLL_CTL_MOD, socket.getFd(), &ev);
}

void WebServer::handleClientWrite(Socket &socket) {
	Client::ClientIterator it = _clients.find(socket.getFd());
	if (it == _clients.end())
		return;
	it->second.sendData();
	LOG("Response sent         | " << it->second);
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, socket.getFd(), NULL);
	socket.close();
	_clients.erase(it);
}

bool WebServer::isWebServerSocket(const Socket &socket) {
	for (size_t i = 0; i < _ServerSockets.size(); ++i)
		if (_ServerSockets[i].getFd() == socket.getFd())
			return true;
	return false;
}

Socket &WebServer::findSocketByFd(int fd) {
	for (size_t i = 0; i < _ServerSockets.size(); ++i) {
		if (_ServerSockets[i].getFd() == fd)
			return _ServerSockets[i];
	}
	Client::ClientIterator it = _clients.find(fd);
	if (it != _clients.end())
		return it->second.getSocket();
	throw std::runtime_error("Socket not found");
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
			Socket &socket = findSocketByFd(events[i].data.fd);
			if (isWebServerSocket(socket)) {
				acceptNewClient(socket);
			} else {
				if (events[i].events & EPOLLIN)
					handleClientRead(socket);
				if (events[i].events & EPOLLOUT)
					handleClientWrite(socket);
			}
		}
		std::memset(&events, 0, sizeof events);
	}
	LOG("WebServer shutting down...");
}
