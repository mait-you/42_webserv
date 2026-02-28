#include "../../includes/Socket.hpp"

Socket::Socket() : _fd(-1), _ip(""), _port(""), _is_bound(false), _is_listening(false) {}

Socket::Socket(int fd) : _fd(-1), _ip(""), _port(""), _is_bound(false), _is_listening(false) {
	if (fd == -1)
		throwError("Socket: invalid file descriptor");
	_fd = fd;
}

Socket::Socket(const std::string& ip, const std::string& port)
		: _fd(-1), _ip(ip), _port(port), _is_bound(false), _is_listening(false) {}

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
	}
	return *this;
}

Socket::~Socket() {}

void Socket::createAndBind() {
	struct addrinfo	 hints;
	struct addrinfo* result;
	struct addrinfo* rp;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family	  = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	  = AI_PASSIVE;

	int status = getaddrinfo(_ip.empty() ? NULL : _ip.c_str(), _port.c_str(), &hints, &result);
	if (status != 0)
		throwError("getaddrinfo: " + std::string(gai_strerror(status)));
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		_fd = ::socket(rp->ai_family, SOCK_STREAM, 0);
		if (_fd == -1)
			continue;
		int opt = 1;
		setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (::bind(_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
			_is_bound = true;
			break;
		}
		::close(_fd);
		_fd = -1;
	}
	freeaddrinfo(result);
	if (!_is_bound)
		throwError("Socket::createAndBind: failed on " + _ip + ":" + _port);
}

void Socket::setNonBlocking() {
	if (_fd == -1)
		throwError("Socket::setNonBlocking: socket not created");
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		throwError(std::string("fcntl F_GETFL: ") + std::strerror(errno));
	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throwError("fcntl F_SETFL: ");
}

void Socket::startListening(int backlog) {
	if (!_is_bound)
		throwError("Socket::startListening: socket not bound");
	if (::listen(_fd, backlog) == -1)
		throwError("listen: ");
	_is_listening = true;
}

Socket Socket::acceptClient() {
	struct sockaddr_in client_addr;
	socklen_t		   addr_len = sizeof(client_addr);
	std::memset(&client_addr, 0, sizeof(client_addr));
	int clientFd = ::accept(_fd, (struct sockaddr*) &client_addr, &addr_len);
	if (clientFd == -1)
		throwError("accept: ");
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
	out << "Socket(fd=" << s.getFd() << ", " << s.getIp() << ":" << s.getPort();
	if (s.isBound())
		out << ", bound";
	if (s.isListening())
		out << ", listening";
	out << ")";
	return out;
}
