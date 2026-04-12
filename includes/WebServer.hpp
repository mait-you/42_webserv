#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "SessionInfo.hpp"
#include "Socket.hpp"

typedef struct epoll_event t_ev;
#define MAX_EVENTS 10
#define EPOLL_EVENT(name)                                                                          \
	t_ev name;                                                                                     \
	std::memset(&name, 0, sizeof(name));

class WebServer {
  public:
	static bool running;

  private:
	Socket::Map		 _serverSockets;
	Client::Map		 _clients;
	SessionInfo::Map _sessions;
	t_ev			 _events[MAX_EVENTS];
	int				 _epollFd;
	const Config&	 _config;

  public:
	WebServer(const Config& conf);
	~WebServer();

	void run();

	const Socket::Map&		getServerSockets() const;
	const Client::Map&		getClients() const;
	const SessionInfo::Map& getSessions() const;
	const Config&			getConfig() const;

  private:
	void acceptClient(Socket& serverSock);
	bool handleRequest(Client& client);
	bool handleResponse(Client& client);
	void removeClient(Client& client);

	void printPrefix();

  private:
	WebServer();
	WebServer(const WebServer& other);
	WebServer& operator=(const WebServer&);
};

std::ostream& operator<<(std::ostream& out, const WebServer& webServer);

#endif
