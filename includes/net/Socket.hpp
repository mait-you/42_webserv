#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "../Head.hpp"
#include "../config/Config.hpp"

class Socket {
  public:
	typedef std::map<int, Socket> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	int					_fd;
	std::string			_ip;
	std::string			_port;
	const ServerConfig* _serverConfig;

  public:
	Socket();
	Socket(int fd);
	Socket(const std::string& ip, const std::string& port, const ServerConfig* serverConfig);
	Socket(const Socket& other);
	Socket& operator=(const Socket& other);
	~Socket();

	void   setup();
	Socket accept();
	void   close();

	void setIp(const std::string& ip);
	void setPort(const std::string& port);

	int					getFd() const;
	const std::string&	getIp() const;
	const std::string&	getPort() const;
	const ServerConfig* getConf() const;
};

#endif
