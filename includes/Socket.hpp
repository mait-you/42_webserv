#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "head.hpp"

class Socket {
  public:
	typedef std::map<int, Socket> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	int			_fd;
	std::string _ip;
	std::string _port;
	bool		_is_bound;
	bool		_is_listening;

  public:
	Socket();
	Socket(int fd); // throws if fd == -1
	Socket(const std::string &ip, const std::string &port);
	Socket(const Socket &other);
	Socket &operator=(const Socket &other);
	~Socket();

	void createAndBind();
	void setNonBlocking();
	void startListening(int backlog);
	Socket acceptClient();
	void   close();

	// Setters
	void setIp(const std::string &ip);
	void setPort(const std::string &port);

	// Getters
	int				   getFd() const;
	const std::string &getIp() const;
	const std::string &getPort() const;
	bool			   isBound() const;
	bool			   isListening() const;
	bool			   isValid() const;
};

std::ostream &operator<<(std::ostream &out, const Socket &socket);

#endif
