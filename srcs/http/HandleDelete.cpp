#include "../../includes/http/Response.hpp"

void Response::deleteFolder(const Request& request, const std::string& fullPath) {
	DIR* dir = opendir(fullPath.c_str());
	if (!dir)
		return (errorPage(request, HTTP_403_FORBIDDEN));

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
			continue;

		std::string entryPath = fullPath + entry->d_name;
		struct stat info;
		if (stat(entryPath.c_str(), &info) != 0) {
			closedir(dir);
			return (errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR));
		}

		if (S_ISREG(info.st_mode)) {
			if (access(entryPath.c_str(), W_OK) == -1) {
				closedir(dir);
				return (errorPage(request, HTTP_403_FORBIDDEN));
			}
			if (std::remove(entryPath.c_str()) != 0) {
				closedir(dir);
				return (errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR));
			}
		} else if (S_ISDIR(info.st_mode)) {
			entryPath += "/";
			deleteFolder(request, entryPath);
			if (_statusCode != HTTP_204_NO_CONTENT) {
				closedir(dir);
				return;
			}
		}
	}
	if (std::remove(fullPath.c_str()) != 0) {
		closedir(dir);
		return (errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR));
	}
	closedir(dir);
	return (errorPage(request, HTTP_204_NO_CONTENT));
}

void Response::handleDelete(const Request& request) {
	struct stat info;

	std::string fullPath = request.getResolveFullPath();
	if (stat(fullPath.c_str(), &info) == -1)
		errorPage(request, HTTP_404_NOT_FOUND);
	else if (S_ISDIR(info.st_mode)) {
		if (fullPath[fullPath.size() - 1] != '/')
			fullPath += '/';
		deleteFolder(request, fullPath);
	} else if (access(fullPath.c_str(), W_OK) == -1)
		errorPage(request, HTTP_403_FORBIDDEN);
	else if (std::remove(fullPath.c_str()) != 0)
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
	else
		setStatus(HTTP_204_NO_CONTENT);
	setBody("");
}
