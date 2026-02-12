#include "../../includes/Socket.hpp"

Socket::Socket() : _fd(-1), _result(NULL) {
	std::memset(&_hints, 0, sizeof(_hints));
	_hints.ai_family   = AF_INET;	  // IPv4 or IPv6
	_hints.ai_socktype = SOCK_STREAM; // TCP socket
	LOG_DEBUGG("Socket", "Default constructor called");
}

Socket::Socket(const Socket &other) {
	LOG_DEBUGG("Socket", "Copy constructor called");
	*this = other;
}

Socket &Socket::operator=(const Socket &other) {
	if (this != &other) {
		_fd		= other._fd;
		_port	= other._port;
		_host	= other._host;
		_hints	= other._hints;
		_result = NULL;
	}
	LOG_DEBUGG("Socket", "Copy assignment constructor called");

	return *this;
}

Socket::~Socket() {
	if (_result)
		freeaddrinfo(_result);
	LOG_DEBUGG("Socket", "Destructor called");
}

void Socket::_create() {
	if (_fd != -1)
		throw std::runtime_error("Socket already has fd");

	int state = getaddrinfo(_host.c_str(), _port.c_str(), &_hints, &_result);
	if (state != 0)
		throw std::runtime_error("getaddrinfo() failed");

	_fd =
		socket(_result->ai_family, _result->ai_socktype, _result->ai_protocol);
	if (_fd < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt() failed");
}

void Socket::_bind() {
	if (bind(_fd, _result->ai_addr, _result->ai_addrlen) < 0)
		throw std::runtime_error("bind() failed");

	if (_result) {
		freeaddrinfo(_result);
		_result = NULL;
	}
}

void Socket::_listen() {
	if (listen(_fd, 5) < 0)
		throw std::runtime_error("listen() failed");
}

void Socket::setNonBlocking() {
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags < 0)
		throw std::runtime_error("fcntl(F_GETFL) failed");

	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}

void Socket::setPort(const std::string &port) {
	_port = port;
}

void Socket::setHost(const std::string &host) {
	_host = host;
}

void Socket::setFd(int fd) {
	_fd = fd;
}

int Socket::getFd() const {
	return _fd;
}

const std::string &Socket::getPort() const {
	return _port;
}

const std::string &Socket::getHost() const {
	return _host;
}

std::ostream &operator<<(std::ostream &out, const Socket &socket) {
	out << "Socket [fd=" << socket.getFd() << ", host=" << socket.getHost()
		<< ", port=" << socket.getPort() << "]";
	return out;
}
