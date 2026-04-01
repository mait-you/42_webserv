#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "SessionInfo.hpp"

class WebServer {
  public:
	static bool running;

  private:
	Socket::Map	  _serverSockets;
	Client::Map	  _clients;
	int			  _epollFd;
	t_ev		  _events[MAX_EVENTS];
	const Config& _config;
	std::map<std::string, SessionInfo> _sessions;

  public:
	WebServer(const Config& conf);
	~WebServer();

	void run();

  private:
	void acceptClient(Socket& serverSock);

	bool handleRead(int fd);

	bool handleWrite(int fd);
	void removeClient(int fd);

  private:
	WebServer();
	WebServer(const WebServer& other);
	// WebServer& operator=(const WebServer& other);
};

#endif
