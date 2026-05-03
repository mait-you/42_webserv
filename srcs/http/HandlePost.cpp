#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

void Response::multiPart(Request& request, const MultipartField& part,
						 const std::string& uploadDir, bool &hasUpload) {
	if (part.filename.empty()) {
		setBody(part.name + ": " + part.data);
		return;
	}
	const std::string filePath = buildFilePath(uploadDir, Mime::getExtension(part.contentType));
	if (!writeFile(filePath, part.data))
		return errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
	setBody(filePath + "\n");
	hasUpload = true;
}

void Response::handleMultipartFields(Request& request, const std::string& uploadDir) {
	const Request::MultipartFields& parts = request.getMultipartFields();
	if (parts.empty())
		return;

	bool		hasUpload = false;
	std::string bodyStr	  = "Form data received successfully.\n";

	for (size_t i = 0; i < parts.size(); ++i) {
		multiPart(request, parts[i], uploadDir, hasUpload);
		if (_statusCode == HTTP_500_INTERNAL_SERVER_ERROR)
			return;
		bodyStr += getBody();
	}

	setStatus(hasUpload ? HTTP_201_CREATED : HTTP_200_OK);
	setHeader("Content-Type", "text/plain");
	setBody(bodyStr + "\n");
}

void Response::handleFormData(Request& request) {
	const Request::FormData& formData = request.getFormData();
	if (formData.empty())
		return;

	std::string bodyStr = "Form data received successfully.\n";
	for (Request::FormData::const_iterator it = formData.begin(); it != formData.end(); ++it) {
		bodyStr += it->first + ": ";
		for (size_t i = 0; i < it->second.size(); ++i) {
			bodyStr += it->second[i];
			if (i < it->second.size() - 1)
				bodyStr += ", ";
		}
		bodyStr += "\n";
	}

	setStatus(HTTP_200_OK);
	setHeader("Content-Type", "text/plain");
	setBody(bodyStr);
}

void Response::handleRawBodyUpload(Request& request, const std::string& uploadDir) {
	const std::string contentType = request.getHeader("content-type");
	if (contentType.empty())
		return;

	const std::string filePath = buildFilePath(uploadDir, Mime::getExtension(contentType));
	if (!writeFile(filePath, request.getBody())) {
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return;
	}

	setStatus(HTTP_201_CREATED);
	setHeader("Content-Type", "text/plain");
	setBody("File uploaded: " + filePath + "\n");
}

void Response::handlePost(Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf)
		return errorPage(request, HTTP_403_FORBIDDEN);

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

	if (!locConf->upload || locConf->upload_path.empty())
		return errorPage(request, HTTP_403_FORBIDDEN);

	const std::string uploadDir = locConf->upload_path;
	if (!request.getMultipartFields().empty())
		handleMultipartFields(request, uploadDir);
	else if (!request.getFormData().empty()) {
		handleFormData(request);
	} else
		handleRawBodyUpload(request, uploadDir);
}
