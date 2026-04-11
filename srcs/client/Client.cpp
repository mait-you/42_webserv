#include "../../includes/Client.hpp"

Client::Client()
		: _socket(), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(), _response() {}

Client::Client(const Socket& socket, const ServerConfig* serverConfig,
			   std::map<std::string, SessionInfo>* session)
		: _socket(socket), _recvBuffer(), _sendBuffer(), _bytesSent(0), _request(serverConfig),
		  _response(session) {}

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
	ssize_t n					  = recv(_socket.getFd(), buf, sizeof(buf), 0);
	if (n <= 0)
		return false;
	_recvBuffer.append(buf, n);
	parseRequest();
	return true;
}

bool Client::parseRequest() {
	return _request.parse(_recvBuffer);
}

bool Client::sendData() {
	buildResponse();
	ssize_t n =
		send(_socket.getFd(), _sendBuffer.c_str() + _bytesSent, _sendBuffer.size() - _bytesSent, 0);
	if (n < 0)
		return false;
	return true;
}

bool Client::buildResponse() {
	if (_response.hasCgiRunning())
		return true;
	_sendBuffer = _response.build(_request);
	if (_sendBuffer.empty())
		return false;
	return true;
}

Socket& Client::getSocket() {
	return _socket;
}

Request& Client::getRequest() {
	return _request;
}

const Request& Client::getRequest() const {
	return _request;
}

Response& Client::getResponse() {
	return _response;
}

const Response& Client::getResponse() const {
	return _response;
}

bool Client::hasCgiRunning() const {
	return _response.hasCgiRunning();
}

void printClient(std::ostream& out, const Client& client, const std::string& connector,
				 const std::string& pre) {
	const Socket&	s	= const_cast<Client&>(client).getSocket();
	const Request&	req = client.getRequest();
	const Response& res = client.getResponse();

	bool hasReq = req.isComplete();
	bool hasRes = res.isComplete();

	out << GRY "│  " RST << connector << s << "\n";

	if (hasReq) {
		std::string reqConn = (hasRes ? GRY "├─ " RST : GRY "└─ " RST);
		out << GRY "│  " RST << pre << reqConn << WHT "Request" RST "\n";
		printRequest(out, req, GRY "│  " RST + pre + (hasRes ? GRY "│   " RST : GRY "    " RST),
					 GRY "└─ " RST);
	}
	if (hasRes) {
		out << GRY "│  " RST << pre << GRY "└─ " RST << WHT "Response" RST "\n";
		printResponse(out, res, GRY "│  " RST + pre + GRY "    " RST, GRY "└─ " RST);
	}
}

std::ostream& operator<<(std::ostream& out, const Client& client) {
	printClient(out, client, GRY "├─ " RST, GRY "│   " RST);
	return out;
}
