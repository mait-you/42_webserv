#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include "head.hpp"

class Client {
  private:
	std::string _buffer;		  // data received from client
	std::string _response;		  // response to send
	bool		_requestComplete; // is request fully received?
	bool		_responseSent;	  // is response sent?

  public:
  Socket _socket; // client socket fd
	Client();
	Client(Socket socket);
	~Client();
	Client(const Client &other);
	Client &operator=(const Client &other);


	void readData(); // read from socket
	void sendData(); // send to socket

	// bool isRequestComplete() const;
	// bool isResponseSent() const;

};

#endif
