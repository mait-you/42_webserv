#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../session/SessionInfo.hpp"
#include "Socket.hpp"

#define RECV_BUFFER_SIZE 4096

class Client {
  public:
	typedef std::map<int, Client> Map;
	typedef Map::iterator		  It;
	typedef Map::const_iterator	  ConstIt;

  private:
	Socket		_socket;
	std::string _recvBuffer;
	std::string _sendBuffer;
	std::size_t _bytesSent;
	Request		_request;
	Response	_response;

  public:
	Client();
	Client(const Socket& socket, const ServerConfig* config,
		   std::map<std::string, SessionInfo>* _session);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();

	bool recvData();
	bool sendData();

	bool parseRequest();
	bool buildResponse();

	bool hasCgiRunning() const;

	Socket&			getSocket();
	Request&		getRequest();
	Response&		getResponse();
	const Request&	getRequest() const;
	const Response& getResponse() const;
};

void		  printClient(std::ostream& out, const Client& client, const std::string& connector,
						  const std::string& pre);
std::ostream& operator<<(std::ostream& out, const Client& client);

#endif
