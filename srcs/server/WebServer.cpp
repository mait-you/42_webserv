#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer() : _epollFd(-1) {
	LOG_DEBUGG("WebServer", "Default constructor called");
}

WebServer::WebServer(const WebServer &) {
	LOG_DEBUGG("WebServer", "Copy constructor called");
}

WebServer &WebServer::operator=(const WebServer &) {
	LOG_DEBUGG("WebServer", "Copy assignment constructor called");
	return *this;
}

WebServer::~WebServer() {
	// Close all listening sockets
	for (size_t i = 0; i < _sockets.size(); ++i)
		close(_sockets[i].getFd());

	// Close all client connections
	for (std::map<int, Client>::iterator it = _clients.begin();
		 it != _clients.end(); it++)
		close(it->first);

	// Close epoll
	if (_epollFd != -1)
		close(_epollFd);

	LOG_DEBUGG("WebServer", "Destructor called");
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
			_sockets.push_back(socket);
			std::cout << socket << std::endl;
		}
	}
}

void WebServer::setupEpoll() {
	// Create epoll instance
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		throw std::runtime_error("epoll_create1() failed");

	// Add all listening sockets to epoll
	for (size_t i = 0; i < _sockets.size(); ++i) {
		struct epoll_event ev;
		ev.events  = EPOLLIN; // Watch for read events
		ev.data.fd = _sockets[i].getFd();

		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
			throw std::runtime_error("epoll_ctl() failed for listening socket");
	}
}

void WebServer::init(const std::string &configFile) {
	_config.parse(configFile);
	std::cout << _config << std::endl;
	setupSockets();
	setupEpoll();
}

void WebServer::acceptNewClient(Socket &socket) {
	socket.accept();
	socket.setNonBlocking();
	Client client(socket.getFd());

	// Add client to epoll (watch for read and write)
	struct epoll_event ev;
	ev.events  = EPOLLIN | EPOLLOUT; // Read and write events
	ev.data.fd = socket.getFd();

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, socket.getFd(), &ev) == -1) {
		std::cout << "Failed to add client to epoll" << std::endl;
		perror("epoll_ctl");
		close(socket.getFd());
		return;
	}

	_clients[socket.getFd()] = client;
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

	// Remove from epoll
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, socket.getFd(), NULL);

	close(socket.getFd());
	_clients.erase(it);
}

bool WebServer::isWebServerSocket(const Socket &socket) const {
	for (size_t i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].getFd() == socket.getFd())
			return true;
	}
	return false;
}

void WebServer::run() {
	std::cout << "WebServer running with..." << std::endl;

	while (WebServer::running) {
		// Wait for events (timeout = -1 means wait forever)
		int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if (numEvents == -1 && errno == EINTR)
			continue; // Interrupted by signal
		if (numEvents == -1)
			throw std::runtime_error("epoll_wait() failed");

		// Process all events
		for (int i = 0; i < numEvents; ++i) {
			Socket socket = events[i].data.fd;

			// Is this a listening socket?
			if (isWebServerSocket(socket)) {
				acceptNewClient(socket);
			}
			// Client socket
			else {
				if (events[i].events & EPOLLIN) {
					handleClientRead(socket);
				}
				if (events[i].events & EPOLLOUT) {
					handleClientWrite(socket);
				}
			}
		}
	}

	std::cout << "WebServer shutting down..." << std::endl;
}
