#include "../../includes/net/Socket.hpp"

#include "../../includes/utils/Logger.hpp"
#include "../../includes/utils/Utils.hpp"

Socket::Socket() : _fd(-1), _ip(""), _port(""), _serverConfig(NULL) {}

Socket::Socket(int fd) : _fd(fd), _ip(""), _port(""), _serverConfig(NULL) {
	if (fd == -1)
		errorLog("Socket constructor", "fd is -1 (invalid socket)");
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
	addrinfo  hints;
	addrinfo *result = NULL, *rp = NULL;
	bool	  bound = false;

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family	  = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	  = AI_PASSIVE;

	int status = getaddrinfo(_ip.empty() ? NULL : _ip.c_str(), _port.c_str(), &hints, &result);
	if (status != 0)
		errorLog("Socket::setup::getaddrinfo", gai_strerror(status));

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
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
		errorLog("Socket::setup", "failed to bind on " + _ip + ":" + _port);
	if (fcntl(_fd, F_SETFD, FD_CLOEXEC) == -1)
		errorLog("Socket::setup::fcntl", "failed to set FD_CLOEXEC");

	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		errorLog("Socket::setup::fcntl", "F_GETFL failed");
	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		errorLog("Socket::setup::fcntl", "F_SETFL O_NONBLOCK failed");

	if (listen(_fd, SOMAXCONN) == -1)
		errorLog("Socket::setup::listen", "failed to listen");
}

Socket Socket::accept() {
	sockaddr_in client_addr;
	socklen_t	addr_len = sizeof client_addr;
	std::memset(&client_addr, 0, sizeof client_addr);

	int clientFd = ::accept(_fd, (sockaddr*) &client_addr, &addr_len);
	if (clientFd == -1)
		errorLog("Socket::accept", "accept() failed");
	if (fcntl(clientFd, F_SETFD, FD_CLOEXEC) == -1) {
		::close(clientFd);
		errorLog("Socket::accept::fcntl", "failed to set FD_CLOEXEC");
	}
	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1)
		errorLog("Socket::setup::fcntl", "F_GETFL failed");
	if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
		errorLog("Socket::setup::fcntl", "F_SETFL O_NONBLOCK failed");
	Socket s(clientFd);
	s.setIp(ipv4Tostr(client_addr.sin_addr.s_addr));
	s.setPort(portTostr(ntohs(client_addr.sin_port)));
	return s;
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

bool Socket::isValid() const {
	return _fd != -1;
}

const ServerConfig* Socket::getConf() const {
	return _serverConfig;
}
