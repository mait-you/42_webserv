#include "../../includes/Client.hpp"

Client::Client()
		: _socket(), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(), _response(),
		  _requestComplete(false), _responseSent(false) {}

Client::Client(const Socket& socket, const ServerConfig* serverConfig, std::map<std::string, SessionInfo>* session)
		: _socket(socket), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(serverConfig),
		  _response(session), _requestComplete(false), _responseSent(false) {}

Client::Client(const Client& other)
		: _socket(other._socket), _recvBuffer(other._recvBuffer), _sendBuffer(other._sendBuffer),
		  _bytesSent(other._bytesSent), _request(other._request), _response(other._response),
		  _requestComplete(other._requestComplete), _responseSent(other._responseSent) {}

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_socket			 = other._socket;
		_recvBuffer		 = other._recvBuffer;
		_sendBuffer		 = other._sendBuffer;
		_bytesSent		 = other._bytesSent;
		_request		 = other._request;
		_response		 = other._response;
		_requestComplete = other._requestComplete;
		_responseSent	 = other._responseSent;
	}
	return *this;
}

Client::~Client() {}

bool Client::readData() {
	char	buf[RECV_BUFFER_SIZE] = {0};
	ssize_t n					  = recv(_socket.getFd(), buf, sizeof(buf), MSG_DONTWAIT);
	if (n <= 0)
		return false;
	_recvBuffer.append(buf, n);
	try {
		_requestComplete = _request.parse(_recvBuffer);
		LOG("Request complete      |\n" << _request);
	} catch (const std::exception& e) {
		_requestComplete = true;
		LOG("Request Parse error   | " << e.what());
	}
	return true;
}

bool Client::sendData() {
	if (!_requestComplete || _responseSent)
		return true;

	if (_sendBuffer.empty())
		_sendBuffer = _response.build(_request);

	ssize_t n = send(_socket.getFd(), _sendBuffer.c_str() + _bytesSent,
					 _sendBuffer.size() - _bytesSent, MSG_DONTWAIT);
	if (n < 0)
		return false;

	_bytesSent += static_cast<std::size_t>(n);
	if (_bytesSent >= _sendBuffer.size()) {
		_responseSent = true;
		LOG("Response sent         | " << *this);
		return false;
	}
	return true;
}

void Client::setResponse(const std::string& response) {
	_sendBuffer	  = response;
	_bytesSent	  = 0;
	_responseSent = false;
}

Request& Client::getRequest() {
	return _request;
}

Socket& Client::getSocket() {
	return _socket;
}

bool Client::isRequestComplete() const {
	return _requestComplete;
}

bool Client::isResponseSent() const {
	return _responseSent;
}

std::ostream& operator<<(std::ostream& out, const Client& client) {
	const Socket& s = const_cast<Client&>(client).getSocket();
	out << "Client(fd=" << s.getFd() << ", " << s.getIp() << ":" << s.getPort() << ")";
	return out;
}
