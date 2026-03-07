#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handleGet(const Request& req, ServerConfig& srv, LocationConfig* locConfig) {
	std::string root;
	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root = srv.root;
	std::string fullPath = root + cleanUri(req.getUri());

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
		return;
	}
	if (S_ISREG(buffer.st_mode))
		handleFile(srv, locConfig, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		handleDir(req, srv, locConfig, fullPath);
	else
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
}
