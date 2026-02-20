#include "../../includes/Client.hpp"

Client::Client()
	: _socket(), _requestComplete(false), _responseSent(false), _bytesSent(0) {
}

Client::Client(const Socket &socket)
	: _socket(socket), _requestComplete(false), _responseSent(false),
	  _bytesSent(0) {
}

Client::Client(const Client &other)
	: _socket(other._socket), _buffer(other._buffer),
	  _response(other._response), _requestComplete(other._requestComplete),
	  _responseSent(other._responseSent), _bytesSent(other._bytesSent) {
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_buffer			 = other._buffer;
		_response		 = other._response;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
		_socket			 = other._socket;
		_bytesSent		 = other._bytesSent;
	}
	return *this;
}

Client::~Client() {
}

void Client::readData() {
	char buffer[1024] = {0};

	ssize_t n = recv(_socket.getFd(), buffer, sizeof buffer - 1, 0);
	if (n <= 0)
		return;
	_buffer += std::string(buffer, n);

	LOG("Request received      | \n");

	if (_buffer.find("\r\n\r\n") == std::string::npos)
		return; // wait for more data

	_request.parse(_buffer);
	_requestComplete = true;
	LOG("Request complet       | \n" << _request);
}

void Client::sendData() {
	_response = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nhello\n";

	if (!_requestComplete)
		return;

	ssize_t n = send(_socket.getFd(), _response.c_str() + _bytesSent,
					 _response.length() - _bytesSent, 0);
	if (n > 0)
		_bytesSent += n;
	_responseSent = (_bytesSent >= _response.length());
	LOG("Response sent         | " << *this);
}

Socket &Client::getSocket() {
	return _socket;
}

bool Client::isRequestComplete() const {
	return _requestComplete;
}

bool Client::isResponseSent() const {
	return _responseSent;
}

std::ostream &operator<<(std::ostream &out, Client &client) {
	out << "Client(fd=" << client.getSocket().getFd() << ", "
		<< client.getSocket().getIp() << ":" << client.getSocket().getPort()
		<< ")";
	return out;
}
