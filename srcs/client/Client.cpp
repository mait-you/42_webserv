#include "../../includes/Client.hpp"

Client::Client() : _socket(), _requestComplete(false), _responseSent(false) {
}

Client::Client(const Socket &socket)
	: _socket(socket), _requestComplete(false), _responseSent(false) {
}

Client::Client(const Client &other)
	: _socket(other._socket), _buffer(other._buffer),
	  _response(other._response), _requestComplete(other._requestComplete),
	  _responseSent(other._responseSent) {
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_buffer			 = other._buffer;
		_response		 = other._response;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
		_socket			 = other._socket;
	}

	return *this;
}

Client::~Client() {
}

void Client::readData() {
	char buffer[1024];

	ssize_t n = recv(_socket.getFd(), buffer, sizeof buffer - 1, 0);
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
	ssize_t n = send(_socket.getFd(), _response.c_str(), _response.length(), 0);
	if (n <= 0)
		return;
}

Socket &Client::getSocket() {
	return _socket;
}

// bool Client::isRequestComplete() const {
// 	return _requestComplete;
// }

// bool Client::isResponseSent() const {
// 	return _responseSent;
// }

std::ostream &operator<<(std::ostream &out, Client &client) {
	out << "Client(fd=" << client.getSocket().getFd() << ", "
		<< client.getSocket().getIp() << ":" << client.getSocket().getPort()
		<< ")";
	return out;
}
