#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "head.hpp"

class Client {
  public:
	typedef std::map<int, Client> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	Socket		_socket;
	std::string _rawBuffer;		  // bytes received so far
	std::string _response;		  // full response string (built once)
	std::size_t _bytesSent;		  // how many bytes of _response already sent
	bool		_requestComplete; // true when a full request is in _rawBuffer
	bool		_responseSent;	  // true when _response is fully sent
	Request		_request;

	bool checkRequestComplete() const;
	void buildResponse();

  public:
	Client();
	Client(const Socket &socket);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	bool readData();
	bool sendData();

	Socket &getSocket();
	bool	isRequestComplete() const;
	bool	isResponseSent() const;
};

std::ostream &operator<<(std::ostream &out, const Client &client);

#endif
