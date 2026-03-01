#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handlePost(Request& req, ServerConfig& srv, LocationConfig* locConfig) {
	if (!locConfig || !locConfig->upload || locConfig->upload_path.empty()) {
		errorPage(srv, locConfig, 403, "Forbidden");
		return;
	}

	std::string body = req.getBody();
	if (body.empty()) {
		errorPage(srv, locConfig, 400, "Bad Request");
		return;
	}

	std::string uploadDir = locConfig->upload_path;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';

	std::ostringstream oss;
	oss << uploadDir << "upload_" << std::time(NULL);

	std::string ext = Mime::getExtension(req.getHeader("Content-Type"));
	if (!ext.empty())
		ext = "." + ext;
	std::string filePath = oss.str() + ext;

	std::ofstream file(filePath.c_str(), std::ios::binary);
	if (!file) {
		errorPage(srv, locConfig, 500, "Internal Server Error");
		return;
	}

	file.write(body.c_str(), body.size());
	file << body;
	file.close();

	setStatus(201, "Created");
	setHeader("Content-Type", "text/plain");
	std::string responseBody = "File uploaded: " + filePath + "\r\n";
	setBody(responseBody);
}
