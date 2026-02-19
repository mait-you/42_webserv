#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Socket.hpp"
#include "head.hpp"

class Client {
  public:
	typedef std::map<int, Client>	  ClientMap;
	typedef ClientMap::iterator		  ClientIterator;
	typedef ClientMap::const_iterator ConstClientIterator;

  private:
	Socket		_socket;		  // client socket fd
	std::string _buffer;		  // data received from client
	std::string _response;		  // response to send
	bool		_requestComplete; // is request fully received?
	bool		_responseSent;	  // is response sent?
	Request		_request;

  public:
	Client();
	Client(const Socket &socket);
	Client(const Client &other);
	~Client();
	Client &operator=(const Client &other);

	void readData(); // read from socket
	void sendData(); // send to socket

	Socket &getSocket();
	bool isRequestComplete() const;
	bool isResponseSent() const;
};

std::ostream &operator<<(std::ostream &out, Client &client);

#endif
