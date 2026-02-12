#include "../../includes/Server.hpp"

bool Server::running = true;

Server::Server() : _epollFd(-1) {
	LOG_DEBUGG("Server", "Default constructor called");
}

Server::Server(const Server &) {
	LOG_DEBUGG("Server", "Copy constructor called");
}

Server &Server::operator=(const Server &) {
	LOG_DEBUGG("Server", "Copy assignment constructor called");
	return *this;
}

Server::~Server() {
	// Close all listening sockets
	for (size_t i = 0; i < _sockets.size(); ++i)
		close(_sockets[i].getFd());

	// Close all client connections
	for (std::map<int, Client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
		close(it->first);

	// Close epoll
	if (_epollFd != -1)
		close(_epollFd);

	LOG_DEBUGG("Server", "Destructor called");
}

void Server::setupSockets() {
	const std::vector<ServerConfig> &servers = _config.getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		for (size_t j = 0; j < servers[i].ports.size(); ++j) {
			Socket socket;

			socket.setHost(servers[i].host);
			socket.setPort(servers[i].ports[j]);

			socket._create();
			socket._bind();
			socket._listen();
			socket.setNonBlocking();

			_sockets.push_back(socket);
		}
	}
}

void Server::setupEpoll() {
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

void Server::init(const std::string &configFile) {
	std::cout << "Loading configuration from: " << configFile << std::endl;
	_config.parse(configFile);
	// std::cout << _config << std::endl;
	setupSockets();
	setupEpoll();
}

void Server::acceptNewClient(int listenFd) {
	Socket clientS;

	int clientFd = accept(listenFd, NULL, NULL);
	if (clientFd < 0)
		return;

	clientS.setFd(clientFd);
	clientS.setNonBlocking();

	Client client(clientS.getFd());

	// Add client to epoll (watch for read and write)
	struct epoll_event ev;
	ev.events  = EPOLLIN | EPOLLOUT; // Read and write events
	ev.data.fd = clientFd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
		std::cout << "Failed to add client to epoll" << std::endl;
		close(clientFd);
		return;
	}

	_clients[clientFd] = client;
	std::cout << "New client connected! FD: " << clientFd << std::endl;
}

void Server::handleClientRead(int clientFd) {
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
		return;

	std::cout << "Request from FD " << clientFd << std::endl;
	it->second.readData();
}

void Server::handleClientWrite(int clientFd) {
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
		return;

	it->second.sendData();

	std::cout << "Response sent to FD " << clientFd << ", closing" << std::endl;

	// Remove from epoll
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);

	close(clientFd);
	_clients.erase(it);
}

bool Server::isServerSocket(int fd) const {
	for (size_t i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].getFd() == fd)
			return true;
	}
	return false;
}

void Server::run() {
	std::cout << "Server running with..." << std::endl;

	struct epoll_event events[10]; // Array to hold events

	while (Server::running) {
		// Wait for events (timeout = -1 means wait forever)
		int numEvents = epoll_wait(_epollFd, events, 10, -1);
		if (numEvents == -1 && errno == EINTR)
			continue; // Interrupted by signal
		if (numEvents == -1)
			throw std::runtime_error("epoll_wait() failed");

		// Process all events
		for (int i = 0; i < numEvents; ++i) {
			int fd = events[i].data.fd;

			// Is this a listening socket?
			if (isServerSocket(fd)) {
				acceptNewClient(fd);
			}
			// Client socket
			else {
				if (events[i].events & EPOLLIN) {
					handleClientRead(fd);
				}
				if (events[i].events & EPOLLOUT) {
					handleClientWrite(fd);
				}
			}
		}
	}

	std::cout << "Server shutting down..." << std::endl;
}
