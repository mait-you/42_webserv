#include "../../includes/net/Socket.hpp"

#include "../../includes/utils/Logger.hpp"
#include "../../includes/utils/Utils.hpp"

Socket::Socket() : _fd(-1), _ip(""), _port(""), _serverConfig(NULL) {}

Socket::Socket(int fd) : _fd(fd), _ip(""), _port(""), _serverConfig(NULL) {
	if (fd == -1)
		throwError("Socket constructor", "fd is -1 (invalid socket)");
}

Socket::Socket(const std::string& ip, const std::string& port, const ServerConfig* serverConfig)
		: _fd(-1), _ip(ip), _port(port), _serverConfig(serverConfig) {}

Socket::Socket(const Socket& other) : _fd(-1), _ip(""), _port(""), _serverConfig(NULL) {
	*this = other;
}

Socket& Socket::operator=(const Socket& other) {
	if (this != &other) {
		_fd			  = other._fd;
		_ip			  = other._ip;
		_port		  = other._port;
		_serverConfig = other._serverConfig;
	}
	return *this;
}

Socket::~Socket() {}

void Socket::setup() {
	addrinfo  filter;
	addrinfo *result = NULL, *rp = NULL;
	bool	  bound = false;

	std::memset(&filter, 0, sizeof filter);
	filter.ai_family   = AF_INET;
	filter.ai_socktype = SOCK_STREAM;
	filter.ai_protocol = IPPROTO_TCP;

	int status = getaddrinfo(_ip.c_str(), _port.c_str(), &filter, &result);
	if (status != 0)
		throwError("Socket::setup::getaddrinfo", gai_strerror(status));

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		_fd =
			socket(rp->ai_family, rp->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC, rp->ai_protocol);
		if (_fd == -1)
			continue;
		int opt = 1;
		if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1) {
			::close(_fd);
			_fd = -1;
			continue;
		}
		if (bind(_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
			bound = true;
			break;
		}
		::close(_fd);
		_fd = -1;
	}
	freeaddrinfo(result);

	if (!bound)
		throwError("Socket::setup", "failed to bind on " + _ip + ":" + _port);

	if (listen(_fd, SOMAXCONN) == -1)
		throwError("Socket::setup::listen", "failed to listen");
}

Socket Socket::accept() {
	sockaddr_in client_addr;
	socklen_t	addr_len = sizeof client_addr;
	std::memset(&client_addr, 0, sizeof client_addr);

	int newFd = ::accept(_fd, (sockaddr*) &client_addr, &addr_len);
	if (newFd == -1)
		throwError("Socket::accept", "accept() failed");

	Socket newClient(newFd);
	if (fcntl(newClient.getFd(), F_SETFL, O_NONBLOCK) == -1)
		throwError("Socket::accept::fcntl", "O_NONBLOCK failed");
	if (fcntl(newClient.getFd(), F_SETFD, FD_CLOEXEC) == -1)
		throwError("Socket::accept::fcntl", "FD_CLOEXEC failed");
	newClient.setIp(ipv4Tostr(client_addr.sin_addr.s_addr));
	newClient.setPort(portTostr(ntohs(client_addr.sin_port)));
	return newClient;
}

void Socket::close() {
	if (_fd != -1) {
		::close(_fd);
		_fd = -1;
	}
}

void Socket::setIp(const std::string& ip) {
	_ip = ip;
}

void Socket::setPort(const std::string& port) {
	_port = port;
}

int Socket::getFd() const {
	return _fd;
}

const std::string& Socket::getIp() const {
	return _ip;
}

const std::string& Socket::getPort() const {
	return _port;
}

const ServerConfig* Socket::getConf() const {
	return _serverConfig;
}
