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
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#ifndef DEBUGGING
#define DEBUGGING 1
#endif

#define MAX_EVENTS 10
#define LOG_FILE "weberv.log"

typedef struct epoll_event t_ev;

#define LOG_DEBUGG(msg)                                                        \
	if (DEBUGGING)                                                             \
		std::cout << msg << std::endl;

void setupSignals();

#endif
