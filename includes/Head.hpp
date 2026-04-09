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

#ifndef DEBUGGING
	#define DEBUGGING 0
#endif

#define RST "\e[0m"
#define GRY "\e[90m"
#define WHT "\e[97m"
#define GRN "\e[92m"
#define YEL "\e[93m"
#define CYN "\e[96m"
#define RED "\e[91m"
#define MGT "\e[95m"

#define HTTP_VERSION "HTTP/1.0"
#define MAX_EVENTS 10
#define RECV_BUFFER_SIZE 4096
#define MAX_URI_LENGTH 8192
#define MAX_BODY_SIZE (1024 * 1024)

#define THROW_ERROR(context, detail) throwError(__FILE__, __LINE__, context, detail)
#define PRINT_WARNING(context, detail) printWarning(__FILE__, __LINE__, context, detail)

#define LOG(msg) std::cout << msg << std::endl
#define LOG_DEBUGG(msg)                                                                            \
	if (DEBUGGING)                                                                                 \
	std::cout << msg << std::endl

void		setupSignals();
bool		isNumber(const std::string& str);
bool		isValidPort(const std::string& str);
std::string ipv4Tostr(uint32_t ip);
std::string portTostr(uint16_t port);
std::string toLower(const std::string& s);
std::string trimStr(const std::string& s);
bool		getLine(const std::string& buf, std::size_t& pos, std::string& line);
std::string getExtension(const std::string& fullPath);
void		throwError(const std::string& file, int line, const std::string& context,
					   const std::string& detail);
void		printWarning(const std::string& file, int line, const std::string& context,
						 const std::string& detail);

template <typename T>
std::string toString(T val) {
	std::ostringstream ss;
	ss << val;
	return ss.str();
}

#endif
