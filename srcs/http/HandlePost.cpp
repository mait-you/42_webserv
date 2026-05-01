#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

static unsigned long uploadCounter = 0;

void Response::multiPart(Request& request, const MultipartField& part, std::string uploadDir) {
	if (part.filename.empty()) {
		request.setFormData(part.name, part.data);
		setStatus(HTTP_200_OK);
		setHeader("Content-Type", "text/plain");
		setBody(part.name + ": " + part.data);
		return;
	}

	std::ostringstream oss;
	oss << uploadDir << "upload_" << std::time(NULL) << "_" << uploadCounter++;

	std::string ext = Mime::getExtension(part.contentType);

	if (!ext.empty())
		ext = "." + ext;

	const std::string filePath = oss.str() + ext;
	std::ofstream	  file(filePath.c_str(), std::ios::binary);
	if (!file) {
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return;
	}

	file.write(part.data.c_str(), part.data.size());
	file.close();

	setStatus(HTTP_201_CREATED);
	setHeader("Content-Type", "text/plain");
	setBody(filePath + "\n");
}


void Response::handlePost(Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	if (request.hasCgi()) {
		const std::string fullPath = request.getResolveFullPath();
		Cgi				  cgi(request, *request.getConf(), locConf, fullPath);
		CgiInfo			  info = cgi.start();
		if (info.pid == -1) {
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
			return;
		}
		_runningCgi	   = info;
		_hasCgiRunning = true;
		return;
	}

	if (!locConf || !locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	std::string uploadDir = locConf->upload_path;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';

	static unsigned long uploadCounter = 0;
	std::string			 type		   = request.getHeader("content-type");
	bool upload = false;
	std::string bodyStr = "Form data received successfully.\n";
	if (type.find("multipart/form-data") != std::string::npos) {
		for (size_t i = 0; i < request.getMultipartFields().size(); i++) {
			multiPart(request, request.getMultipartFields()[i], uploadDir);
			if (getStatusCode() == HTTP_201_CREATED)
				upload = true;
			if (getStatusCode() != HTTP_201_CREATED && getStatusCode() != HTTP_200_OK)
				return;
			bodyStr += getBody();
		}
		if (upload)
			setStatus(HTTP_201_CREATED);
		setBody(bodyStr + "\n");
	} else if (type == "application/x-www-form-urlencoded") {
		setStatus(HTTP_200_OK);
		setHeader("Content-Type", "text/plain");
		Request::FormData data = request.getFormData();
		for (Request::FormData::const_iterator it = data.begin(); it != data.end(); ++it) {
			bodyStr += it->first + ": " + it->second + "\n";
		}
		setBody(bodyStr);
	} else {
		std::ostringstream oss;
		oss << uploadDir << "upload_" << std::time(NULL) << "_" << uploadCounter++;

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
		setStatus(HTTP_201_CREATED);
		setHeader("Content-Type", "text/plain");
		setBody("File uploaded: " + filePath + "\n");
	}
}
