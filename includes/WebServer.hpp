#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Socket.hpp"
#include "head.hpp"
#include "Response.hpp"

class WebServer {

  private:
	static bool running;

  private:
	Socket::Map _ServerSock;
	Client::Map _clients;
	Config		_config;
	int			_epollFd;
	t_ev		events[MAX_EVENTS];

  public:
	WebServer();
	~WebServer();

	void init(const std::string &configFile);
	void run();
	ServerConfig matchedServer(Request &req);
	LocationConfig* matchedLocation(ServerConfig &srv, Request &req);

	static void stop(int);

  private:
	void	acceptNewClient(Socket &ServerSock);
	void	handleClientRead(int fd);
	void	handleClientWrite(int fd);

	WebServer(const WebServer &other);
	WebServer &operator=(const WebServer &other);
};

#endif
