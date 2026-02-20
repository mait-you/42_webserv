#include "../includes/WebServer.hpp"
#include "../includes/head.hpp"

int main(int ac, char **av) {

	if (ac > 2) {
		std::cerr << "Error: Too many arguments" << std::endl;
		std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
		return 1;
	}
	std::string configFile = (ac == 2) ? av[1] : "config/default.conf";
	setupSignals();
	try {
		WebServer webServer;
		webServer.init(configFile);
		webServer.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
