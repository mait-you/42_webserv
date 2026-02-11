#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "webserv.hpp"

class Socket {
  private:
	int				 _fd;	// socket file descriptor
	std::string		 _port; // port number
	std::string		 _host; // host address
	struct addrinfo	 _hints;
	struct addrinfo *_result;

  public:
	Socket();
	Socket(const Socket &other);
	Socket &operator=(const Socket &other);
	~Socket();

	void _create(); // create socket
	void _bind();	// bind to port
	void _listen(); // start listening
	void _accept(); // accept connection (returns new socket)

	int				   getFd() const;
	const std::string &getPort() const;
	const std::string &getHost() const;

	void setPort(const std::string &port);
	void setHost(const std::string &host);
	void setFd(int fd);	   // set file descriptor
	void setNonBlocking(); // make socket non-blocking
};


#endif
