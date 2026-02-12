#include "../includes/Server.hpp"
#include "../includes/webserv.hpp"

void signalHandler(int) {
	Server::running = false;
}

void setupSignals() {
	signal(SIGINT, signalHandler);
}

int main(int ac, char **av, char **env) {
	std::string configFile;
	(void)env; // cgi
	if (ac > 2) {
		std::cerr << "Error: Too many arguments" << std::endl;
		std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
		return 1;
	}
	if (ac == 2)
		configFile = av[1];
	else
		configFile = "config/default.conf";
	setupSignals();
	try {
		Server server;
		server.init(configFile);
		std::cout << "Server started successfully Press Ctrl+C to stop"
				  << std::endl;
		server.run();
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
