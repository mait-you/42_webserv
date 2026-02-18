#include "../../includes/Socket.hpp"

Socket::Socket()
	: _fd(-1), _ip(""), _port(""), _is_bound(false), _is_listening(false) {
}

Socket::Socket(int fd)
	: _fd(fd), _ip(""), _port(""), _is_bound(false), _is_listening(false) {
	if (fd == -1)
		throw std::runtime_error("Socket: invalid file descriptor");
}

Socket::Socket(const std::string &host, const std::string &port)
	: _fd(-1), _ip(host), _port(port), _is_bound(false), _is_listening(false) {
}

Socket::Socket(const Socket &other) {
	*this = other;
}

// useed only in push_back()
Socket &Socket::operator=(const Socket &other) {
	if (this != &other) {
		_fd			  = other._fd;
		_ip			  = other._ip;
		_port		  = other._port;
		_is_bound	  = other._is_bound;
		_is_listening = other._is_listening;
	}
	return *this;
}

Socket::~Socket() {
}

void Socket::createAndBind() {
	struct addrinfo hints, *result, *rp;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family	  = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	  = AI_PASSIVE;

	int status = getaddrinfo(_ip.empty() ? NULL : _ip.c_str(), _port.c_str(),
							 &hints, &result);
	if (status != 0)
		throw std::runtime_error("getaddrinfo failed");
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		_fd = socket(rp->ai_family, SOCK_STREAM, 0);
		if (_fd == -1)
			continue;
		int opt = 1;
		if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
			-1) {
			::close(_fd);
			_fd = -1;
			continue;
		}
		if (::bind(_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
			_is_bound = true;
			break;
		}
		::close(_fd);
		_fd = -1;
	}

	freeaddrinfo(result);

	if (!_is_bound)
		throw std::runtime_error("Failed to create and bind socket");
}

void Socket::setNonBlocking() {
	if (_fd == -1)
		throw std::runtime_error("Socket not created");

	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl F_GETFL failed");

	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl F_SETFL failed");
}

void Socket::listen(int backlog) {
	if (!_is_bound)
		throw std::runtime_error("Socket must be bound before listen");

	if (::listen(_fd, backlog) == -1)
		throw std::runtime_error("listen failed");

	_is_listening = true;
}

Socket Socket::accept() {
	if (!_is_listening)
		return Socket(-1);
	struct sockaddr_in client_addr;
	socklen_t		   addr_len = sizeof(client_addr);
	std::memset(&client_addr, 0, sizeof(client_addr));
	int clientFd = ::accept(_fd, (struct sockaddr *) &client_addr, &addr_len);
	Socket clientSock(clientFd);
	clientSock.setIp(ipv4Tostr(client_addr.sin_addr.s_addr));
	clientSock.setPort(portTostr(ntohs(client_addr.sin_port)));
	return clientSock;
}

void Socket::close() {
	if (_fd != -1) {
		::close(_fd);
		_fd = -1;
	}
}

void Socket::setIp(const std::string &host) {
	_ip = host;
}

void Socket::setPort(const std::string &port) {
	_port = port;
}

int Socket::getFd() const {
	return _fd;
}

const std::string &Socket::getIp() const {
	return _ip;
}

const std::string &Socket::getPort() const {
	return _port;
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

std::ostream &operator<<(std::ostream &out, const Socket &socket) {
	out << "Socket(fd=" << socket.getFd() << ", " << socket.getIp() << ":"
		<< socket.getPort();

	if (socket.isBound())
		out << ", bound";
	if (socket.isListening())
		out << ", listening";

	out << ")";
	return out;
}
