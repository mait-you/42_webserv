#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#ifndef DEBUGGING
#define DEBUGGING 0
#endif

#define MAX_EVENTS 10

typedef struct epoll_event t_ev;
#define EPOLL_EVENT(name)                                                      \
	t_ev name;                                                                 \
	std::memset(&name, 0, sizeof(name));

#define LOG_DEBUGG(msg)                                                        \
	if (DEBUGGING)                                                             \
		std::cout << msg << std::endl;

#include <cerrno>
#include <cstring>
#include <ctime>
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
#include <sys/stat.h>

inline std::string getTime() {
	std::time_t t = std::time(NULL);
	char		buf[20];
	std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
	return std::string(buf);
}

#define LOG(msg) std::cout << "[" << getTime() << "] " << msg << std::endl

void setupSignals();
bool isNumber(std::string str);
bool isValidPort(std::string str);

std::string ipv4Tostr(uint32_t ip);
std::string portTostr(uint16_t port);

std::string getLine(const std::string &raw, size_t &pos);
std::string trim(const std::string &s);

#endif
