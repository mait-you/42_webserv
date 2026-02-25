#include "../../includes/Client.hpp"

Client::Client()
	: _socket(), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(),
	  _response(), _config(), _requestComplete(false), _responseSent(false) {
}

Client::Client(const Socket &socket, const Config &conf)
	: _socket(socket), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(),
	  _response(), _config(conf), _requestComplete(false),
	  _responseSent(false) {
}

Client::Client(const Client &other)
	: _socket(other._socket), _recvBuffer(other._recvBuffer),
	  _sendBuffer(other._sendBuffer), _bytesSent(other._bytesSent),
	  _request(other._request), _response(other._response),
	  _config(other._config), _requestComplete(other._requestComplete),
	  _responseSent(other._responseSent) {
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_socket			 = other._socket;
		_recvBuffer		 = other._recvBuffer;
		_sendBuffer		 = other._sendBuffer;
		_bytesSent		 = other._bytesSent;
		_request		 = other._request;
		_response		 = other._response;
		_config			 = other._config;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
	}
	return *this;
}

Client::~Client() {
}

bool Client::checkRequestComplete() const {
	std::size_t headerEnd = _recvBuffer.find(END_OF_HEADERS);
	if (headerEnd == std::string::npos)
		return false;
	std::string headers = _recvBuffer.substr(0, headerEnd);
	if (headers.find("Transfer-Encoding: chunked") != std::string::npos)
		return _recvBuffer.find("0" END_OF_HEADERS) != std::string::npos;
	std::size_t clPos = headers.find("Content-Length: ");
	if (clPos != std::string::npos) {
		std::size_t		   lineEnd = headers.find("\r\n", clPos + 16);
		std::size_t		   bodyLen = 0;
		std::istringstream iss(
			headers.substr(clPos + 16, lineEnd - (clPos + 16)));
		iss >> bodyLen;
		return (_recvBuffer.size() - (headerEnd + 4)) >= bodyLen;
	}
	return true;
}

bool Client::readData() {
	char	buf[RECV_BUFFER_SIZE];
	ssize_t n = recv(_socket.getFd(), buf, sizeof(buf), MSG_DONTWAIT);
	if (n <= 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	_recvBuffer += std::string(buf, n);
	if (!_requestComplete && checkRequestComplete()) {
		_requestComplete = true;
		_request.parse(_recvBuffer);
		_request.validate();
		LOG("Request complete      |\n" << _request);
	}
	return true;
}

bool Client::sendData() {
	if (!_requestComplete || _responseSent)
		return true;

	if (_sendBuffer.empty())
		_sendBuffer = _response.build(_request, _config.getServers());

	ssize_t n = send(_socket.getFd(), _sendBuffer.c_str() + _bytesSent,
					 _sendBuffer.size() - _bytesSent, MSG_DONTWAIT);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		return false;
	}
	_bytesSent += n;
	if (_bytesSent >= _sendBuffer.size()) {
		_responseSent = true;
		LOG("Response sent         | " << *this);
		return false;
	}
	return true;
}

void Client::setResponse(const std::string &response) {
	_sendBuffer	  = response;
	_bytesSent	  = 0;
	_responseSent = false;
}

Request &Client::getRequest() {
	return _request;
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
