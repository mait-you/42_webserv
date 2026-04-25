#include "../../includes/net/Client.hpp"
#include "../../includes/utils/Utils.hpp"

// it call only for Map
Client::Client()
		: _socket(), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(), _response() {}

Client::Client(const Socket& socket, const Socket& serverSock,
			   std::map<std::string, SessionInfo>* session)
		: _socket(socket), _recvBuffer(), _sendBuffer(), _bytesSent(0),
		  _request(&serverSock, socket.getIp()), _response(session) {}

Client::Client(const Client& other)
		: _socket(other._socket), _recvBuffer(other._recvBuffer), _sendBuffer(other._sendBuffer),
		  _bytesSent(other._bytesSent), _request(other._request), _response(other._response) {}

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_socket		= other._socket;
		_recvBuffer = other._recvBuffer;
		_sendBuffer = other._sendBuffer;
		_bytesSent	= other._bytesSent;
		_request	= other._request;
		_response	= other._response;
	}
	return *this;
}

Client::~Client() {}

bool Client::recvData() {
	char	buf[RECV_BUFFER_SIZE] = {0};
	ssize_t n					  = recv(_socket.getFd(), buf, sizeof buf, 0);
	if (n == 0)
		return false;
	if (n < 0)
		return true;
	_recvBuffer.append(buf, n);

	// printEscaped(_recvBuffer);

	return true;
}

bool Client::parseRequest() {
	return _request.parse(_recvBuffer);
}

bool Client::sendData() {
	if (_sendBuffer.empty() || _bytesSent >= _sendBuffer.size())
		return false;

	ssize_t n =
		send(_socket.getFd(), _sendBuffer.c_str() + _bytesSent, _sendBuffer.size() - _bytesSent, 0);
	if (n == 0)
		return false;
	if (n < 0)
		return true;

	_bytesSent += static_cast<std::size_t>(n);
	if (_bytesSent >= _sendBuffer.size()) {
		_sendBuffer.clear();
		_bytesSent = 0;
		return false;  // fully sent → close
	}
	return true;  // partial send → keep sending
}

bool Client::buildResponse() {
	_response = _request;
	if (_response.hasCgiRunning()) {
		if (!_response.pollCgi(_request))
			return false;  // CGI not done yet
		_sendBuffer = _response.buildSendBuffer();
		return true;  // CGI done, buffer ready
	}
	_sendBuffer = _response.build(_request);
	return !_response.hasCgiRunning();	// false only if CGI just started
}

Socket& Client::getSocket() {
	return _socket;
}
Request& Client::getRequest() {
	return _request;
}
Response& Client::getResponse() {
	return _response;
}

const Socket& Client::getSocket() const {
	return _socket;
}
const Request& Client::getRequest() const {
	return _request;
}
const Response& Client::getResponse() const {
	return _response;
}

bool Client::hasCgiRunning() const {
	return _response.hasCgiRunning();
}
