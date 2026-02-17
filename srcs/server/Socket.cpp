#include "../../includes/Socket.hpp"

Socket::Socket()
	: _fd(-1), _host(""), _port(""), _family(AF_UNSPEC), _is_bound(false),
	  _is_listening(false) {
}

Socket::Socket(int fd)
	: _fd(fd), _host(""), _port(""), _family(AF_UNSPEC), _is_bound(false),
	  _is_listening(false) {
	if (fd < 0)
		throw std::runtime_error("Socket: invalid file descriptor");
}

Socket::Socket(const std::string &host, const std::string &port)
	: _fd(-1), _host(host), _port(port), _family(AF_UNSPEC), _is_bound(false),
	  _is_listening(false) {
}

// Socket::Socket(const Socket &other)
// 	: _fd(other._fd), _host(other._host), _port(other._port),
// 	  _family(other._family), _is_bound(other._is_bound),
// 	  _is_listening(other._is_listening) {
// }

Socket::Socket(const Socket &other) {
	*this = other;
}

// useed only in push_back()
Socket &Socket::operator=(const Socket &other) {
	if (this != &other) {
		_fd			  = other._fd;
		_host		  = other._host;
		_port		  = other._port;
		_family		  = other._family;
		_is_bound	  = other._is_bound;
		_is_listening = other._is_listening;
	}
	return *this;
}

Socket::~Socket() {
}

void Socket::setupSocketopt(int family) {
	int opt = 1;

	// Enable address reuse
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("setsockopt SO_REUSEADDR failed");

	// For IPv6, accept both IPv4 and IPv6 connections
	if (family == AF_INET6) {
		int no = 0;
		if (setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) == -1)
			throw std::runtime_error("setsockopt IPV6_V6ONLY failed");
	}
}

void Socket::createAndBind() {
	struct addrinfo	 hints;
	struct addrinfo *result;
	struct addrinfo *rp;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family	  = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	  = AI_PASSIVE;
	hints.ai_protocol = 0;

	const char *node   = _host.empty() ? NULL : _host.c_str();
	int			status = getaddrinfo(node, _port.c_str(), &hints, &result);
	if (status != 0)
		throw std::runtime_error("getaddrinfo failed");
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		_fd = socket(rp->ai_family, SOCK_STREAM, 0);
		if (_fd == -1)
			continue;
		_family = rp->ai_family;
		try {
			setupSocketopt(rp->ai_family);
		} catch (std::exception &e) {
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

int Socket::accept() {
	if (!_is_listening)
		return -1;

	struct sockaddr_storage client_addr;
	socklen_t				addr_len = sizeof(client_addr);

	int client_fd = ::accept(_fd, (struct sockaddr *) &client_addr, &addr_len);
	return client_fd;
}

void Socket::close() {
	if (_fd != -1) {
		::close(_fd);
		_fd = -1;
	}
}

void Socket::setHost(const std::string &host) {
	_host = host;
}

void Socket::setPort(const std::string &port) {
	_port = port;
}

int Socket::getFd() const {
	return _fd;
}

const std::string &Socket::getHost() const {
	return _host;
}

const std::string &Socket::getPort() const {
	return _port;
}

int Socket::getFamily() const {
	return _family;
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
	out << "Socket(fd=" << socket.getFd() << ", " << socket.getHost() << ":"
		<< socket.getPort();

	if (socket.getFamily() == AF_INET)
		out << ", IPv4";
	else if (socket.getFamily() == AF_INET6)
		out << ", IPv6";

	if (socket.isBound())
		out << ", bound";
	if (socket.isListening())
		out << ", listening";

	out << ")";
	return out;
}
