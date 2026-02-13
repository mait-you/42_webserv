#include "../../includes/Client.hpp"

Client::Client() : _requestComplete(false), _responseSent(false), _socket() {
}

Client::Client(Socket socket)
	: _requestComplete(false), _responseSent(false), _socket(socket) {
}

Client::Client(const Client &other)
	: _buffer(other._buffer), _response(other._response),
	  _requestComplete(other._requestComplete),
	  _responseSent(other._responseSent), _socket(other._socket) {
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

	ssize_t n = recv(_socket.getFd(), buffer, sizeof(buffer) - 1, 0);
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

// bool Client::isRequestComplete() const {
// 	return _requestComplete;
// }

// bool Client::isResponseSent() const {
// 	return _responseSent;
// }
