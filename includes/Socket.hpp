#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "head.hpp"

class Socket {
  private:
	int			_fd;
	std::string _host;
	std::string _port;
	int			_family;
	bool		_is_bound;
	bool		_is_listening;

	void setupSocket(int family);

  public:
	Socket();
	Socket(int fd);
	Socket(const std::string &host, const std::string &port);
	Socket(const Socket &other);
	Socket &operator=(const Socket &other);
	~Socket();

	void createAndBind();
	void setNonBlocking();
	void listen(int backlog);
	int	 accept();
	void close();

	int				   getFd() const;
	const std::string &getHost() const;
	const std::string &getPort() const;
	int				   getFamily() const;
	bool			   isBound() const;
	bool			   isListening() const;
	bool			   isValid() const;
};

std::ostream &operator<<(std::ostream &out, const Socket &socket);

#endif
