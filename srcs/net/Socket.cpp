#include "../../includes/net/Socket.hpp"

#include "../includes/utils/Logger.hpp"
#include "../includes/utils/Utils.hpp"

Socket::Socket() : _fd(-1), _ip(""), _port(""), _is_bound(false), _is_listening(false) {}

Socket::Socket(int fd) : _fd(-1), _ip(""), _port(""), _is_bound(false), _is_listening(false) {
	if (fd == -1)
		ERROR_LOG("Socket constructor", "fd is -1 (invalid socket)");
	_fd = fd;
}

Socket::Socket(const std::string& ip, const std::string& port, const ServerConfig* serverConfig)
		: _fd(-1), _ip(ip), _port(port), _is_bound(false), _is_listening(false),
		  _serverConfig(serverConfig) {}

Socket::Socket(const Socket& other) {
	*this = other;
}

Socket& Socket::operator=(const Socket& other) {
	if (this != &other) {
		_fd			  = other._fd;
		_ip			  = other._ip;
		_port		  = other._port;
		_is_bound	  = other._is_bound;
		_is_listening = other._is_listening;
		_serverConfig = other._serverConfig;
	}
	return *this;
}

Socket::~Socket() {}

void Socket::createAndBind() {
	if (_fd != -1)
		ERROR_LOG("Socket::createAndBind", "Socket already " + toString(_fd));
	struct addrinfo	 hints;
	struct addrinfo *result = NULL, *rp = NULL;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family	  = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	  = AI_PASSIVE;

	int status = getaddrinfo(_ip.empty() ? NULL : _ip.c_str(), _port.c_str(), &hints, &result);
	if (status != 0)
		ERROR_LOG("getaddrinfo", gai_strerror(status));
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
			_is_bound = true;
			break;
		}
		::close(_fd);
		_fd = -1;
	}
	freeaddrinfo(result);
	if (!_is_bound)
		ERROR_LOG("Socket::createAndBind", "failed to bind on " + _ip + ":" + _port);
	if (fcntl(_fd, F_SETFD, FD_CLOEXEC) == -1)
		ERROR_LOG("Socket::createAndBind::fcntl", "failed to set FD_CLOEXEC");
}

void Socket::setNonBlocking() {
	if (_fd == -1)
		ERROR_LOG("Socket::setNonBlocking", "socket not created");
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		ERROR_LOG("fcntl", "F_GETFL failed");
	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		ERROR_LOG("fcntl", "F_SETFL O_NONBLOCK failed");
}

void Socket::startListening(int backlog) {
	if (!_is_bound)
		ERROR_LOG("Socket::startListening", "socket is not bound");
	if (listen(_fd, backlog) == -1)
		ERROR_LOG("listen", "failed to start listening");
	_is_listening = true;
}

Socket Socket::accept() {
	struct sockaddr_in client_addr;
	socklen_t		   addr_len = sizeof(client_addr);
	std::memset(&client_addr, 0, sizeof(client_addr));

	int clientFd = ::accept(_fd, (struct sockaddr*) &client_addr, &addr_len);
	if (clientFd == -1)
		ERROR_LOG("Socket::accept", "accept() failed");
	if (fcntl(clientFd, F_SETFD, FD_CLOEXEC) == -1) {
		::close(clientFd);
		ERROR_LOG("fcntl", "failed to set FD_CLOEXEC");
	}
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
const ServerConfig* Socket::getServerConf() const {
	return _serverConfig;
}
bool Socket::isBound() const {
	return _is_bound;
}
bool Socket::isListening() const {
	return _is_listening;
}
bool Socket::isValid() const {
	return _fd != -1;
}

std::ostream& operator<<(std::ostream& out, const Socket& s) {
	out << GRY "[" RST;
	out << CYN "fd=" RST << YEL << s.getFd() << RST;
	out << GRY "] " RST;
	out << WHT << s.getIp() << RST;
	out << GRY ":" RST;
	out << WHT << s.getPort() << RST;
	if (s.isBound())
		out << GRY " · " RST GRN "bound" RST;
	if (s.isListening())
		out << GRY " · " RST GRN "listening" RST;
	return out;
}
