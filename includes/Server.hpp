#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"
#include "Socket.hpp"
#include "webserv.hpp"

class Server {
  private:
	std::vector<Socket>	  _sockets;
	std::map<int, Client> _clients;
	Config				  _config;
	int					  _epollFd;

  public:
	Server();
	~Server();

	void init(const std::string &configFile);
	void run();

	static bool running;

  private:
	void setupSockets();
	void setupEpoll();
	void acceptNewClient(int listenFd);
	void handleClientRead(int clientFd);
	void handleClientWrite(int clientFd);
	bool isServerSocket(int fd) const;

	Server(const Server &other);
	Server &operator=(const Server &other);
};

#endif
