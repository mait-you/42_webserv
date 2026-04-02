#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handlePost(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	// CGI check FIRST — before upload guard
	if (request.hasCgi()) {
		const std::string fullPath = request.resolveFullPath();
		Cgi				  cgi(request, *request.getServerConf(), locConf, fullPath);
		CgiInfo			  info = cgi.start();
		if (info.pid == -1) {
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
			return;
		}
		_runningCgi	   = info;
		_hasCgiRunning = true;
		return;
	}

	// upload guard AFTER CGI check
	if (!locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
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

	const std::string& body = request.getBody();
	file.write(body.c_str(), body.size());
	file.close();

	setStatus(HTTP_201_CREATED, "Created");
	setHeader("Content-Type", "text/plain");
	setBody("File uploaded: " + filePath + "\n");
}
