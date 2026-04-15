#include "../includes/net/WebServer.hpp"
#include "../includes/utils/Utils.hpp"

int main(int ac, char** av) {
	if (ac > 2) {
		std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
		return 1;
	}

	std::string configFile = (ac == 2) ? av[1] : "config/default.conf";
	setupSignals();

	try {
		Config	  config(configFile);
		WebServer server(config);
		server.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
