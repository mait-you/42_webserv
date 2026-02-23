#ifndef BUILD_RESPONSE_HPP
#define BUILD_RESPONSE_HPP

#include "WebServer.hpp"

Response buildResponse(Request &req, ServerConfig &srv, LocationConfig *locConfig);

#endif
