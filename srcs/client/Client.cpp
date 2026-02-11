#include "../../includes/Client.hpp"

Client::Client() : _fd(-1), _requestComplete(false), _responseSent(false) {
	LOG_DEBUGG("Client", "Default constructor called");
}

Client::Client(const int fd)
	: _fd(fd), _requestComplete(false), _responseSent(false) {
	LOG_DEBUGG("Client", "Parameterized constructor called");
}

Client::Client(const Client &other) {
	LOG_DEBUGG("Client", "Copy constructor called");
	*this = other;
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_fd				 = other._fd;
		_buffer			 = other._buffer;
		_response		 = other._response;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
	}
	LOG_DEBUGG("Client", "Copy assignment constructor called");
	return *this;
}

Client::~Client() {
	LOG_DEBUGG("Client", "Destructor called");
}

void Client::readData() {
	char buffer[1024];

	ssize_t n = recv(_fd, buffer, sizeof(buffer) - 1, 0);
	if (n <= 0)
		return;
	buffer[n] = 0;
	_buffer += buffer;
	_response = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nhello\n";

	// std::cout << "{" << std::endl;
	// std::cout << _buffer << std::endl;
	// std::cout << "}" << std::endl;
}

void Client::sendData() {
	ssize_t n = send(_fd, _response.c_str(), _response.length(), 0);
	if (n <= 0)
		return;
}

int Client::getFd() const {
	return _fd;
}

// bool Client::isRequestComplete() const {
// 	return _requestComplete;
// }

// bool Client::isResponseSent() const {
// 	return _responseSent;
// }
