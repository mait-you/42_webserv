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

ServerConfig WebServer::matchedServer(Request &req)
{
	std::string port = "8080";
	std::string server_name;
	std::string host = req.getHeader("Host");
	const ServerConfig *matchedPort = NULL;

	size_t pos = host.find(':');
	if (pos != std::string::npos)
	{
		server_name = host.substr(0, pos);
		port = host.substr(pos + 1);
	}
	const std::vector<ServerConfig> &servers = _config.getServers();
	for (size_t i = 0; i < servers.size(); i++)
	{
		for (size_t j = 0; j < servers[i].ports.size(); j++)
		{
			if (servers[i].ports[j] == port)
			{
				if (!matchedPort)
					matchedPort = &servers[i];
				if (servers[i].server_name == server_name)
					return servers[i];
			}
		}
	}
	if(matchedPort)
		return *matchedPort;
	return servers[0];
}

LocationConfig* WebServer::matchedLocation(ServerConfig &srv, Request &req)
{
	LocationConfig *matched = NULL;
	const std::string &uri = req.getUri();
	size_t matchedLen = 0;

	for (size_t i = 0; i < srv.locations.size(); i++)
	{
		std::string &path = srv.locations[i].path;
		if (uri.compare(0, path.size(), path) == 0)
		{
			if (path.size() > matchedLen)
			{
				matchedLen = path.size();
				matched = &srv.locations[i];
			}
		}
	}
	return matched;
}


void WebServer::handleClientWrite(int fd) {
	Client &client = _clients[fd];

	ServerConfig srv = matchedServer(client.getRequest());
	LocationConfig *locConfig = matchedLocation(srv, client.getRequest());

	Response res = buildResponse(client.getRequest(), srv, locConfig);

	client.setResponse(res.build());
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
