#include "../../includes/Response.hpp"

void Response::deleteFolder(const std::string &fullPath, ServerConfig &srv,
							LocationConfig *locConfig) {
	std::cout << "fullPath " << fullPath << std::endl;
	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
		return (errorPage(srv, locConfig, HTTP_403_FORBIDDEN));

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
			continue;

		std::string entryPath = fullPath + entry->d_name;
		std::cout << "entryPath " << entryPath << std::endl;
		struct stat info;
		if (stat(entryPath.c_str(), &info) != 0) {
			closedir(dir);
			return (
				errorPage(srv, locConfig, HTTP_500_INTERNAL_SERVER_ERROR));
		}

		if (S_ISREG(info.st_mode)) {
			if (access(entryPath.c_str(), W_OK) == -1) {
				closedir(dir);
				return (errorPage(srv, locConfig, HTTP_403_FORBIDDEN));
			}
			if (std::remove(entryPath.c_str()) != 0) {
				closedir(dir);
				return (errorPage(srv, locConfig, HTTP_500_INTERNAL_SERVER_ERROR));
			}
		} else if (S_ISDIR(info.st_mode)) {
			entryPath += "/";
			deleteFolder(entryPath, srv, locConfig);
			if (_statusCode != HTTP_204_NO_CONTENT) {
				closedir(dir);
				return;
			}
		}
	}
	if (std::remove(fullPath.c_str()) != 0) {
		closedir(dir);
		return (errorPage(srv, locConfig, HTTP_500_INTERNAL_SERVER_ERROR));
	}
	closedir(dir);
	return (errorPage(srv, locConfig, HTTP_204_NO_CONTENT));
}

void Response::handleDelete(const Request &req, ServerConfig &srv, LocationConfig *locConfig) {
	struct stat info;
	std::string root;

	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root = srv.root;
	std::string fullPath = root + cleanUri(req.getUri());
	std::cout << "uri conflect " << req.getUri() << std::endl;
	if (stat(fullPath.c_str(), &info) == -1)
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
	else if (S_ISDIR(info.st_mode)) {
		std::cout << "fullPath conflect " << fullPath << std::endl;
		if (fullPath[fullPath.size() - 1] == '/')
			deleteFolder(fullPath, srv, locConfig);
		// else
		// 	errorPage(srv, locConfig, 409);
	} else if (access(fullPath.c_str(), W_OK) == -1)
		errorPage(srv, locConfig, HTTP_403_FORBIDDEN);
	else if (std::remove(fullPath.c_str()) != 0)
		errorPage(srv, locConfig, HTTP_500_INTERNAL_SERVER_ERROR);
	else
		setStatus(HTTP_204_NO_CONTENT, "No Content");
	setBody("");
}
