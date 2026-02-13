#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer() : _epollFd(-1) {
}

WebServer::~WebServer() {
	for (size_t i = 0; i < _ServerSockets.size(); ++i)
		_ServerSockets[i].close();
	for (std::map<int, Client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
		close(it->first);
	if (_epollFd != -1)
		close(_epollFd);
}

void WebServer::stop(int) {
	WebServer::running = false;
}

void WebServer::setupSockets() {
	const std::vector<ServerConfig> &webServers = _config.getServers();
	for (size_t i = 0; i < webServers.size(); ++i) {
		const ServerConfig &server = webServers[i];
		for (size_t j = 0; j < server.ports.size(); ++j) {
			Socket socket(server.host, server.ports[j]);
			socket.create();
			socket.setReuseAddr();
			socket.setNonBlocking();
			socket.bind();
			socket.listen(128);
			_ServerSockets.push_back(socket);
			LOG_DEBUGG("[WebServer] :" << socket);
		}
	}
}

void WebServer::setupEpoll() {
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		throw std::runtime_error("epoll_create1() failed");

	for (size_t i = 0; i < _ServerSockets.size(); ++i) {
		t_ev ev;
		ev.events  = EPOLLIN;
		ev.data.fd = _ServerSockets[i].getFd();

		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
			throw std::runtime_error("epoll_ctl() failed for listening socket");
	}
}

void WebServer::init(const std::string &configFile) {
	_config.parse(configFile);
	LOG_DEBUGG("[WebServer] :" << _config);
	setupSockets();
	setupEpoll();
}

void WebServer::acceptNewClient(Socket &socket) {
	Client client(socket.accept());
	if (client._socket.getFd() == -1)
		return;

	LOG_DEBUGG("[WebServer] : client fd :" << client._socket.getFd());

	client._socket.setNonBlocking();
	t_ev ev;
	ev.events  = EPOLLIN | EPOLLOUT;
	ev.data.fd = client._socket.getFd();
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, client._socket.getFd(), &ev) == -1) {
		std::cout << "Failed to add client to epoll" << std::endl;
		return;
	}
	_clients[client._socket.getFd()] = client;
	std::cout << "New client connected! FD: " << socket.getFd() << std::endl;
}

void WebServer::handleClientRead(Socket &socket) {
	std::map<int, Client>::iterator it = _clients.find(socket.getFd());
	if (it == _clients.end())
		return;
	std::cout << "Request from FD " << socket.getFd() << std::endl;
	it->second.readData();
}

void WebServer::handleClientWrite(Socket &socket) {
	std::map<int, Client>::iterator it = _clients.find(socket.getFd());
	if (it == _clients.end())
		return;
	it->second.sendData();
	std::cout << "Response sent to FD " << socket.getFd() << ", closing"
			  << std::endl;
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, socket.getFd(), NULL);
	socket.close();
	_clients.erase(it);
}

bool WebServer::isWebServerSocket(const Socket &socket) const {
	for (size_t i = 0; i < _ServerSockets.size(); ++i) {
		if (_ServerSockets[i].getFd() == socket.getFd())
			return true;
	}
	return false;
}

void WebServer::run() {
	std::cout << "WebServer running..." << std::endl;

	while (WebServer::running) {
		int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait() failed");
		}
		LOG_DEBUGG("[WebServer] : numEvents :" << numEvents);
		for (int i = 0; i < numEvents; i++) {
			Socket socket = events[i].data.fd;
			if (isWebServerSocket(socket)) {
				acceptNewClient(socket);
			} else {
				if (events[i].events & EPOLLIN)
					handleClientRead(socket);
				if (events[i].events & EPOLLOUT)
					handleClientWrite(socket);
			}
		}
	}
	std::cout << "WebServer shutting down..." << std::endl;
}
