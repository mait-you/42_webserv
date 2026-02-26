#include "../../includes/Response.hpp"

void Response::deleteFolder(const std::string &fullPath, ServerConfig &srv, LocationConfig *locConfig)
{
	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
		return (errorPage(srv, locConfig, 403, "Forbidden"));

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
			continue;

		std::string entryPath = fullPath + entry->d_name;

		struct stat info;
		if (stat(entryPath.c_str(), &info) != 0)
		{
			closedir(dir);
			return (errorPage(srv, locConfig, 500, "Internal Server Error"));
		}

		if (S_ISREG(info.st_mode))
		{
			if (access(entryPath.c_str(), W_OK) == -1)
			{
				closedir(dir);
				return (errorPage(srv, locConfig, 403, "Forbidden"));
			}
			if (std::remove(entryPath.c_str()) != 0)
			{
				closedir(dir);
				return (errorPage(srv, locConfig, 500, "Internal Server Error"));
			}
		}
		else if (S_ISDIR(info.st_mode))
		{
			deleteFolder(entryPath, srv, locConfig);
			if (_statusCode != 204)
			{
				closedir(dir);
				return;
			}
		}
	}
	if (std::remove(fullPath.c_str()) != 0)
	{
		closedir(dir);
		return (errorPage(srv, locConfig, 500, "Internal Server Error"));
	}
	closedir(dir);
	return (errorPage(srv, locConfig, 204, "No Content"));
}

void Response::handleDelete(Request &req, ServerConfig &srv, LocationConfig *locConfig)
{
	struct stat info;
	std::string root;

	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root= srv.root;
	std::string fullPath = root + cleanUri(req.getUri());

	if (stat(fullPath.c_str(), &info) == -1 )
		errorPage(srv, locConfig, 404, "Not Found");
	else if (S_ISDIR(info.st_mode))
	{
		if (fullPath[fullPath.size() - 1] == '/')
		{
			deleteFolder(fullPath, srv, locConfig);
		}
		errorPage(srv, locConfig, 409, "Conflict");
	}
	else if (access(fullPath.c_str(), W_OK) == -1)
		errorPage(srv, locConfig, 403, "Forbidden");
	else if (std::remove(fullPath.c_str()) != 0)
		errorPage(srv, locConfig, 500, "Internal Server Error");
	else
		setStatus(204, "No Content");
}
