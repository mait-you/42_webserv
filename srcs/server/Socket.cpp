#include "../../includes/Socket.hpp"

Socket::Socket() : _fd(-1), _host(""), _port("") {
	std::memset(&_address, 0, sizeof(_address));
}
Socket::Socket(int fd) : _fd(fd), _host(""), _port("") {
	if (fd < 0)
		std::runtime_error("Socket failed to craeat from fd");

	std::memset(&_address, 0, sizeof(_address));
	// Get socket address info using getsockname
	struct sockaddr_in addr;
	socklen_t		   len = sizeof(addr);

	if (getsockname(_fd, (struct sockaddr *) &addr, &len) == 0) {
		memcpy(&_address, &addr, sizeof(_address));

		// Get port using ntohs
		std::stringstream ss;
		ss << ntohs(addr.sin_port);
		_port = ss.str();

		// Convert IP manually using ntohl
		unsigned long	  ip = ntohl(addr.sin_addr.s_addr);
		std::stringstream ip_ss;
		ip_ss << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
			  << ((ip >> 8) & 0xFF) << "." << (ip & 0xFF);
		_host = ip_ss.str();
	}
}

Socket::Socket(const std::string &host, const std::string &port)
	: _fd(-1), _host(host), _port(port) {
	std::memset(&_address, 0, sizeof(_address));
}

Socket::Socket(const Socket &other)
	: _fd(dup(other._fd)), _host(other._host), _port(other._port) {
	memcpy(&_address, &other._address, sizeof(_address));
}

Socket &Socket::operator=(const Socket &other) {
	if (this != &other) {
		if (_fd != -1)
			::close(_fd);
		_fd	  = ::dup(other._fd);
		_host = other._host;
		_port = other._port;
		std::memset(&_address, 0, sizeof(_address));
	}
	return *this;
}

Socket::~Socket() {
	if (_fd != -1)
		::close(_fd);
}

void Socket::create() {
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd == -1)
		throw std::runtime_error("Failed to create socket");
}

void Socket::setNonBlocking() {
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("Failed to get socket flags");
	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to set non-blocking mode");
}

void Socket::setReuseAddr() {
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("Failed to set SO_REUSEADDR");
}

void Socket::bind() {
	struct addrinfo	 hints;
	struct addrinfo *result;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family	  = AF_INET;	 // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags	  = AI_PASSIVE;	 // For bind

	int status = getaddrinfo(_host.c_str(), _port.c_str(), &hints, &result);
	if (status != 0) {
		throw std::runtime_error("getaddrinfo failed: " +
								 std::string(gai_strerror(status)));
	}

	struct addrinfo *rp;
	int				 bindSuccess = -1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		bindSuccess = ::bind(_fd, rp->ai_addr, rp->ai_addrlen);
		if (bindSuccess == 0) {
			memcpy(&_address, rp->ai_addr, sizeof(_address));
			break;
		}
	}

	freeaddrinfo(result); // Free the linked list

	if (bindSuccess == -1)
		throw std::runtime_error("Failed to bind socket to " + _host + ":" +
								 _port);
}

void Socket::listen(int backlog) {
	if (::listen(_fd, backlog) == -1)
		throw std::runtime_error("Failed to listen on socket");
}

int Socket::accept() {
	int client_fd = ::accept(_fd, NULL, NULL);
	if (client_fd == -1)
		std::cout << "Failed to accept a Client" << std::endl;
	return client_fd;
}

void Socket::close() {
	if (_fd != -1) {
		::close(_fd);
		_fd = -1;
	}
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

std::ostream &operator<<(std::ostream &out, const Socket &socket) {
	out << "Socket [fd=" << socket.getFd() << ", host=" << socket.getHost()
		<< ", port=" << socket.getPort() << "]";
	return out;
}
