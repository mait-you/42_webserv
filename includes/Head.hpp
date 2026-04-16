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

#include <cerrno>
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
#include <cctype>

#define RST "\e[0m"
#define GRY "\e[90m"
#define WHT "\e[97m"
#define GRN "\e[92m"
#define YEL "\e[93m"
#define CYN "\e[96m"
#define RED "\e[91m"
#define MGT "\e[95m"

#define HTTP_VERSION "HTTP/1.0"

#define ERROR_LOG(context, detail) throwError(__FILE__, __LINE__, context, detail)
#define WARNING_LOG(context, detail) printWarning(__FILE__, __LINE__, context, detail)

#endif
