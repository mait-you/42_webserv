#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Socket.hpp"
#include "head.hpp"

class WebServer {
  private:
	std::vector<Socket>	  _sockets;
	std::map<int, Client> _clients;
	Config				  _config;
	int					  _epollFd;
	struct epoll_event	  events[MAX_EVENTS];

	static bool running;

  public:
	WebServer();
	~WebServer();

	void init(const std::string &configFile);
	void run();

	static void stop(int);

  private:
	void setupSockets();
	void setupEpoll();
	void acceptNewClient(Socket &socket);
	void handleClientRead(Socket &socket);
	void handleClientWrite(Socket &socket);
	bool isWebServerSocket(const Socket &socket) const;

	WebServer(const WebServer &other);
	WebServer &operator=(const WebServer &other);
};


#endif
