#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#ifndef DEBUGGING
#define DEBUGGING 0
#endif

#define MAX_EVENTS 10

typedef struct epoll_event t_ev;
#define EPOLL_EVENT(name)                                                      \
	t_ev name;                                                                 \
	std::memset(&name, 0, sizeof(name))

#define LOG_DEBUGG(msg)                                                        \
	if (DEBUGGING)                                                             \
		std::cout << msg << std::endl;

#include <cerrno>
#include <cstring>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

void		setupSignals();
std::string ipv4Tostr(uint32_t ip);
std::string ipv6Tostr(const uint8_t ip[16]);
std::string portTostr(uint16_t port);

#endif
