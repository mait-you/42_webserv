#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

int Response::handleErrorFile(const std::string& fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1)
		return 0;
	std::ifstream file(fullPath.c_str());
	if (!file.is_open())
		return 0;
	std::stringstream ss;
	ss << file.rdbuf();
	// ! file.close(); // zt lik hadi
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
	return 1;
}
