#include "../../includes/WebServer.hpp"
#include "../../includes/head.hpp"

void setupSignals() {
	signal(SIGINT, WebServer::stop);
}
