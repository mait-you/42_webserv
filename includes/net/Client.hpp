#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "Socket.hpp"

#define RECV_BUFFER_SIZE 4096

class Client {
  public:
	typedef std::map<int, Client> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	Socket		_socket;
	Socket		_serverSock;
	std::string _recvBuffer;
	std::string _sendBuffer;
	std::size_t _bytesSent;
	Request		_request;
	Response	_response;

  public:
	Client();
	Client(const Socket& socket, const Socket& serverSock,
		   std::map<std::string, std::string>* _session);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();

	bool recvData();
	bool sendData();

	bool parseRequest();
	bool buildResponse();

	bool hasCgiRunning() const;

	Socket&	  getSocket();
	Request&  getRequest();
	Response& getResponse();

	const Socket&	getSocket() const;
	const Request&	getRequest() const;
	const Response& getResponse() const;
};

#endif
