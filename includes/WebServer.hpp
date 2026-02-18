#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Socket.hpp"
#include "head.hpp"

class WebServer {
  private:
	typedef std::map<int, Client> tClientMap;
	typedef tClientMap::iterator  tClientIt;

  private:
	std::vector<Socket> _ServerSockets;
	tClientMap			_clients;
	Config				_config;
	int					_epollFd;
	t_ev				events[MAX_EVENTS];

	static bool running;

  public:
	WebServer();
	~WebServer();

	void init(const std::string &configFile);
	void run();

	static void stop(int);

  private:
	Socket &findSocketByFd(int fd);
	void	acceptNewClient(Socket &socketClient);
	void	handleClientRead(Socket &socket);
	void	handleClientWrite(Socket &socket);
	bool	isWebServerSocket(const Socket &socket);

	WebServer(const WebServer &other);
	WebServer &operator=(const WebServer &other);
};

#endif
