#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handleGet(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();

	std::string root;
	if (locConf && !locConf->root.empty())
		root = locConf->root;
	else
		root = request.getServerConf()->root;
	std::string fullPath = root + cleanUri(request.getUri());

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(request, HTTP_404_NOT_FOUND);
		return;
	}
	if (S_ISREG(buffer.st_mode))
		handleFile(request, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		handleDir(request, fullPath);
	else
		errorPage(request, HTTP_404_NOT_FOUND);
}
