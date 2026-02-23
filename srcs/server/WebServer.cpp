#include "../../includes/WebServer.hpp"

bool WebServer::running = true;

WebServer::WebServer() : _epollFd(-1) {
	std::memset(_events, 0, sizeof(_events));
}

WebServer::~WebServer() {
	for (Socket::It it = _serverSockets.begin(); it != _serverSockets.end();
		 ++it)
		it->second.close();
	for (Client::It it = _clients.begin(); it != _clients.end(); ++it)
		it->second.getSocket().close();

	if (_epollFd != -1)
		::close(_epollFd);
}

void WebServer::stop(int) {
	WebServer::running = false;
}

void WebServer::removeClient(int fd) {
	Client::It it = _clients.find(fd);
	if (it == _clients.end())
		return;

	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	it->second.getSocket().close();
	_clients.erase(it);
}

void WebServer::acceptClient(Socket &serverSock) {
	Socket newSock;
	try {
		newSock = serverSock.acceptClient();
	} catch (const std::exception &e) {
		std::cerr << "accept error: " << e.what() << std::endl;
		return;
	}
	try {
		newSock.setNonBlocking();
	} catch (const std::exception &e) {
		std::cerr << "setNonBlocking error: " << e.what() << std::endl;
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
	_clients[newSock.getFd()] = Client(newSock);
	LOG("New client            | " << newSock);
}

bool WebServer::handleRead(int fd) {
	Client &client = _clients[fd];
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

ServerConfig WebServer::matchedServer(Request &req) {
	std::string			port = "8080";
	std::string			server_name;
	std::string			host		= req.getHeader("Host");
	const ServerConfig *matchedPort = NULL;

	size_t pos = host.find(':');
	if (pos != std::string::npos) {
		server_name = host.substr(0, pos);
		port		= host.substr(pos + 1);
	}
	const std::vector<ServerConfig> &servers = _config.getServers();
	for (size_t i = 0; i < servers.size(); i++) {
		for (size_t j = 0; j < servers[i].ports.size(); j++) {
			if (servers[i].ports[j] == port) {
				if (!matchedPort)
					matchedPort = &servers[i];
				if (servers[i].server_name == server_name)
					return servers[i];
			}
		}
	}
	if (matchedPort)
		return *matchedPort;
	return servers[0];
}

LocationConfig *WebServer::matchedLocation(ServerConfig &srv, Request &req) {
	LocationConfig	  *matched	  = NULL;
	const std::string &uri		  = req.getUri();
	size_t			   matchedLen = 0;

	for (size_t i = 0; i < srv.locations.size(); i++) {
		std::string &path = srv.locations[i].path;
		if (uri.compare(0, path.size(), path) == 0) {
			if (path.size() > matchedLen) {
				matchedLen = path.size();
				matched	   = &srv.locations[i];
			}
		}
	}
	return matched;
}

bool WebServer::handleWrite(int fd) {
	Client &client = _clients[fd];

	ServerConfig	srv		  = matchedServer(client.getRequest());
	LocationConfig *locConfig = matchedLocation(srv, client.getRequest());

	Response res = buildResponse(client.getRequest(), srv, locConfig);

	client.setResponse(res.build());
	if (!client.sendData())
		return false;
	return true;
}

void WebServer::init(const std::string &configFile) {
	_config.parse(configFile);
	_epollFd = epoll_create(true);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("epoll_create: ") +
								 std::strerror(errno));
	const std::vector<ServerConfig> &servers = _config.getServers();
	for (std::size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig &srv = servers[i];
		for (std::size_t j = 0; j < srv.ports.size(); ++j) {
			Socket sock(srv.host, srv.ports[j]);
			sock.createAndBind();
			sock.setNonBlocking();
			sock.startListening(128);
			EPOLL_EVENT(ev);
			ev.events  = EPOLLIN;
			ev.data.fd = sock.getFd();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, sock.getFd(), &ev) == -1)
				throw std::runtime_error(std::string("epoll_ctl: ") +
										 std::strerror(errno));

			_serverSockets[sock.getFd()] = sock;
			LOG("Listening             | " << sock);
		}
	}
}

void WebServer::run() {
	LOG("WebServer running...");
	while (WebServer::running) {
		int numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
		if (numEvents == -1) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error(std::string("epoll_wait: ") +
									 std::strerror(errno));
		}
		for (int i = 0; i < numEvents; ++i) {
			int fd = _events[i].data.fd;
			if (_serverSockets.count(fd)) {
				acceptClient(_serverSockets[fd]);
				continue;
			}
			bool keep = true;
			if (_events[i].events & EPOLLIN)
				keep = handleRead(fd);
			if (keep && (_events[i].events & EPOLLOUT))
				keep = handleWrite(fd);
			if (!keep)
				removeClient(fd);
		}
	}
	LOG("WebServer shutting down...");
}
