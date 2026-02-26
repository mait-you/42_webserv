#include "../../includes/Response.hpp"

// std::string clean(std::string uri)
// {
// 	std::istringstream ss(uri);
// 	std::string buffer;

// 	while (std::getline(ss, buffer, '/'))
// 	{

// 	}
	
// }
void Response::handleDelete(Request &req, ServerConfig &srv, LocationConfig *locConfig)
{
	struct stat fielInfo;
	std::string root;

	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root= srv.root;
	std::string fullPath = root + req.getUri();

	if (stat(fullPath.c_str(), &fielInfo) == -1 )
		errorPage(srv, locConfig, 404, "Not Found");
	else if (S_ISDIR(fielInfo.st_mode))
		errorPage(srv, locConfig, 403, "Forbidden");
	else if (access(fullPath.c_str(), W_OK) == -1)
		errorPage(srv, locConfig, 403, "Forbidden");
	else if (std::remove(fullPath.c_str()) != 0)
		errorPage(srv, locConfig, 500, "Internal Server Error");
	else
		setStatus(204, "No Content");
}
