#ifndef WEBSERV_HPP
#define WEBSERV_HPP

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
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <sstream>

#ifndef DEBUGGING
#define DEBUGGING 0
#endif

#define MAX_EVENTS 10

#define LOG_DEBUGG(className, msg)                                             \
	if (DEBUGGING)                                                             \
		std::cout << "[" << className << "]: " << msg << std::endl;

void setupSignals();
bool isNumber(std::string str);
bool isValidPort(std::string str);

#endif
