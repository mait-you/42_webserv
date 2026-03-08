#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::handlePost(const Request &request) {
	const LocationConfig* locConf = request.getLocationConf();

	if (!locConf || !locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	std::string body = request.getBody();
	if (body.empty()) {
		errorPage(request, HTTP_400_BAD_REQUEST);
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
	std::string filePath = oss.str() + ext;

	std::ofstream file(filePath.c_str(), std::ios::binary);
	if (!file) {
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return;
	}

	file.write(body.c_str(), body.size());
	file << body;
	file.close();

	setStatus(HTTP_201_CREATED, "Created");
	setHeader("Content-Type", "text/plain");
	std::string responseBody = "File uploaded: " + filePath + "\r\n";
	setBody(responseBody);
}
