#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Client.hpp"

class WebServer {
  public:
	static bool								   running;
	typedef std::map<std::string, std::string> Sessions;

  private:
	Socket::Map	  _serverSockets;
	Client::Map	  _clients;
	Sessions	  _sessions;
	t_ev		  _events[MAX_EVENTS];
	int			  _epollFd;
	const Config& _config;

  public:
	WebServer(const Config& conf);
	~WebServer();

	void run();

	const Socket::Map& getServerSockets() const;
	const Client::Map& getClients() const;
	const Sessions&	   getSessions() const;
	const Config&	   getConfig() const;

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
