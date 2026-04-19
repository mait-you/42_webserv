#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "../Head.hpp"

#define ERROR_LOG(context, detail) errorLog(__FILE__, __LINE__, context, detail)
#define WARNING_LOG(context, detail) warnLog(__FILE__, __LINE__, context, detail)

void errorLog(const std::string& file, int line, const std::string& context,
			  const std::string& detail);
void warnLog(const std::string& file, int line, const std::string& context,
			 const std::string& detail);

class WebServer;
class Client;
class Socket;
class Request;
class Response;

void printPrefix(const WebServer& ws);
void printEvent(const WebServer& ws, const char* event);

#endif
