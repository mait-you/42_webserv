#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "head.hpp"

class Socket {
  private:
	int			_fd;
	std::string _ip;
	std::string _port;
	bool		_is_bound;
	bool		_is_listening;

  public:
	Socket();
	Socket(int fd); // throw
	Socket(const std::string &ip, const std::string &port);
	~Socket();

	void   createAndBind();
	void   setNonBlocking();
	void   listen(int backlog);
	Socket accept(); // throw
	void   close();

	void setIp(const std::string &ip);
	void setPort(const std::string &port);

	int				   getFd() const;
	const std::string &getIp() const;
	const std::string &getPort() const;
	bool			   isBound() const;
	bool			   isListening() const;
	bool			   isValid() const;

	//   private:
	Socket(const Socket &other);
	Socket &operator=(const Socket &other);
};

std::ostream &operator<<(std::ostream &out, const Socket &socket);

#endif
