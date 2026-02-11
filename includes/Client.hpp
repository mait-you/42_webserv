#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "webserv.hpp"

class Client {
  private:
	int			_fd;			  // client socket fd
	std::string _buffer;		  // data received from client
	std::string _response;		  // response to send
	bool		_requestComplete; // is request fully received?
	bool		_responseSent;	  // is response sent?

  public:
	Client();
	Client(const int fd);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	void readData(); // read from socket
	void sendData(); // send to socket

	int getFd() const;
	// bool isRequestComplete() const;
	// bool isResponseSent() const;
};

#endif
