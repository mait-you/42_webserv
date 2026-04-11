#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "SessionInfo.hpp"
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
	bool		_requestComplete;
	bool		_responseSent;

  public:
	Client();
	Client(const Socket& socket, const ServerConfig* config,
		   std::map<std::string, SessionInfo>* _session);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();

	bool readData();
	bool sendData();

	void setResponse(const std::string& response);

	bool hasCgiRunning() const;
	bool isRequestComplete() const;
	bool isResponseSent() const;

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
