#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "BuildResponse.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "head.hpp"

class WebServer {
  private:
	static bool running;

	Socket::Map _serverSockets;
	Client::Map _clients;
	Config		_config;
	int			_epollFd;
	t_ev		_events[MAX_EVENTS];

  public:
	WebServer();
	~WebServer();

	void			init(const std::string &configFile);
	void			run();

	static void stop(int signo);

  private:
	void acceptClient(Socket &serverSock);

	bool handleRead(int fd);

	bool handleWrite(int fd);
	void removeClient(int fd);

  private:
	WebServer(const WebServer &other);
	WebServer &operator=(const WebServer &other);
};

Response buildResponse(Request &req, const std::vector<ServerConfig> &servers, Client &client);

#endif
