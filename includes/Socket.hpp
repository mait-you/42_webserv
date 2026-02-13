#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "head.hpp"

class Socket {
  private:
	int				   _fd;
	std::string		   _host;
	std::string		   _port;
	struct sockaddr_in _address;

  public:
	Socket();
	Socket(int fd);
	Socket(const std::string &host, const std::string &port);
	Socket(const Socket &other);
	Socket &operator=(const Socket &other);
	~Socket();

	void		create();
	void		setNonBlocking();
	void		setReuseAddr();
	void		bind();
	void		listen(int backlog);
	int			accept();
	void		close();

	int				   getFd() const;
	const std::string &getHost() const;
	const std::string &getPort() const;

  private:
};

std::ostream &operator<<(std::ostream &out, const Socket &socket);

#endif
