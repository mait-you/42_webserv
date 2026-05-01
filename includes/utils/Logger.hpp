#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "../Head.hpp"

void throwError(const std::string& context, const std::string& detail);
void warnLog(const std::string& context, const std::string& detail);

class WebServer;
class Client;
class Socket;
class Request;
class Response;

void logServerStart(const WebServer& ws);
void logServerEvent(const WebServer& ws, const char* event);

#endif
