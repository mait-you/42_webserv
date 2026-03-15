#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handlePost(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf || !locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	if (request.hasCgi()) {
		const std::string fullPath = request.resolveFullPath();

		Cgi cgi(*_currentRequest, *request.getServerConf(), locConf, fullPath);
		_runningCgi = cgi.start(_clientFd);
		if (_runningCgi.pid == -1)
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		else
			_hasCgiRunning = true;
		return;
	}

	std::string uploadDir = locConf->upload_path;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';
	std::ostringstream oss;
	oss << uploadDir << "upload_" << std::time(NULL);

	std::string ext = Mime::getExtension(request.getHeader("Content-Type"));
	if (!ext.empty())
		ext = "." + ext;
	const std::string filePath = oss.str() + ext;
	std::ofstream	  file(filePath.c_str(), std::ios::binary);
	if (!file) {
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return;
	}

	file.write(request.getBody().c_str(), request.getBody().size());
	file.close();

	setStatus(HTTP_201_CREATED, "Created");
	setHeader("Content-Type", "text/plain");
	setBody("File uploaded: " + filePath + "\r\n");
}
