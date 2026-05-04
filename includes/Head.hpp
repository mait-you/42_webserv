#ifndef HEAD_HPP
#define HEAD_HPP

#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define RST "\e[0m"
#define GRY "\e[90m"
#define WHT "\e[97m"
#define GRN "\e[92m"
#define YEL "\e[93m"
#define CYN "\e[96m"
#define RED "\e[91m"
#define MGT "\e[95m"

typedef epoll_event t_ev;

#define CLIENT_IDLE_TIMEOUT 5

#define MAX_URI_LENGTH 8192

#define MAX_EVENTS 4096

#define EPOLL_EVENT(name)                                                                          \
	t_ev name;                                                                                     \
	std::memset(&name, 0, sizeof name);

#endif
