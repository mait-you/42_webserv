#include "../includes/WebServer.hpp"

int main(int ac, char **av) {
	if (ac > 2) {
		std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
		return 1;
	}

	std::string configFile = (ac == 2) ? av[1] : "config/default.conf";
	setupSignals();

	try {
		WebServer server;
		server.init(configFile);
		server.run();
	} catch (const std::exception &e) {
		std::cerr << "Fatal: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
