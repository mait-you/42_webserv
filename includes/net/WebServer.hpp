#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Client.hpp"

typedef epoll_event t_ev;
#define MAX_EVENTS 1024
#define EPOLL_EVENT(name)                                                                          \
	t_ev name;                                                                                     \
	std::memset(&name, 0, sizeof name);

class WebServer {
  public:
	static bool running;

  private:
	Socket::Map						   _serverSockets;
	Client::Map						   _clients;
	std::map<std::string, std::string> _sessions;
	t_ev							   _events[MAX_EVENTS];
	int								   _epollFd;
	const Config&					   _config;

  public:
	WebServer(const Config& conf);
	~WebServer();

	void run();

	const Socket::Map&						  getServerSockets() const;
	const Client::Map&						  getClients() const;
	const std::map<std::string, std::string>& getSessions() const;
	const Config&							  getConfig() const;

  private:
	void acceptClient(Socket& serverSock);
	bool handleRequest(Client& client);
	bool handleResponse(Client& client);
	void removeClient(Client& client);
	void checkIdleClients();

  private:
	WebServer();
	WebServer(const WebServer&);
	WebServer& operator=(const WebServer&);
};

#endif
