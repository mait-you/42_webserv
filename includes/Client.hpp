#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Socket.hpp"
#include "head.hpp"

class Client {
  public:
	typedef std::map<int, Client> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	Socket		_socket;		  // client socket fd
	std::string _buffer;		  // data received from client
	std::string _response;		  // response to send
	bool		_requestComplete; // is request fully received?
	bool		_responseSent;	  // is response sent?
	std::size_t _bytesSent;
	Request		_request;
	// Response	_response;

  public:
	Client();
	Client(const Socket &socket);
	Client(const Client &other);
	~Client();
	Client &operator=(const Client &other);

	void readData(); // read from socket
	void sendData(); // send to socket

	Socket &getSocket();
	Request &getRequest();
	bool	isRequestComplete() const;
	bool	isResponseSent() const;
};

std::ostream &operator<<(std::ostream &out, Client &client);

#endif
