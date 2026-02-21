#include "../../includes/Client.hpp"

Client::Client()
	: _socket(), _bytesSent(0), _requestComplete(false), _responseSent(false) {
}

Client::Client(const Socket &socket)
	: _socket(socket), _bytesSent(0), _requestComplete(false),
	  _responseSent(false) {
}

Client::Client(const Client &other)
	: _socket(other._socket), _rawBuffer(other._rawBuffer),
	  _response(other._response), _bytesSent(other._bytesSent),
	  _requestComplete(other._requestComplete),
	  _responseSent(other._responseSent), _request(other._request) {
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_socket			 = other._socket;
		_rawBuffer		 = other._rawBuffer;
		_response		 = other._response;
		_bytesSent		 = other._bytesSent;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
		_request		 = other._request;
	}
	return *this;
}

Client::~Client() {
}

bool Client::checkRequestComplete() const {
	std::size_t headerEnd = _rawBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return false;
	std::string headers = _rawBuffer.substr(0, headerEnd);
	if (headers.find("Transfer-Encoding: chunked") != std::string::npos)
		return _rawBuffer.find("0\r\n\r\n") != std::string::npos;
	std::size_t clPos = headers.find("Content-Length: ");
	if (clPos != std::string::npos) {
		std::size_t		   lineEnd = headers.find("\r\n", clPos + 16);
		std::size_t		   bodyLen = 0;
		std::istringstream iss(
			headers.substr(clPos + 16, lineEnd - (clPos + 16)));
		iss >> bodyLen;
		return (_rawBuffer.size() - (headerEnd + 4)) >= bodyLen;
	}

	return true;
}

void Client::buildResponse() {
	Response resp;
	resp.setStatus(200, "OK");
	resp.setHeader("Content-Type", "text/plain");
	resp.setBody("hello\n");
	_response = resp.build();
}

bool Client::readData() {
	char	buf[RECV_BUFFER_SIZE];
	ssize_t n = recv(_socket.getFd(), buf, sizeof(buf), 0);
	if (n == 0)
		return false;
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	_rawBuffer += std::string(buf, static_cast<std::size_t>(n));
	if (!_requestComplete && checkRequestComplete()) {
		_requestComplete = true;
		_request.parse(_rawBuffer);
		buildResponse();
		LOG("Request complete      |");
		// LOG("Request complete      |\n" << _request);
	}
	return true;
}

bool Client::sendData() {
	if (!_requestComplete || _responseSent)
		return true;
	ssize_t n = send(_socket.getFd(), _response.c_str() + _bytesSent,
					 _response.size() - _bytesSent, 0);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	_bytesSent += static_cast<std::size_t>(n);
	if (_bytesSent >= _response.size()) {
		_responseSent = true;
		LOG("Response sent         | " << *this);
		return false;
	}
	return true;
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

std::ostream &operator<<(std::ostream &out, const Client &client) {
	const Socket &s = const_cast<Client &>(client).getSocket();
	out << "Client(fd=" << s.getFd() << ", " << s.getIp() << ":" << s.getPort()
		<< ")";
	return out;
}
