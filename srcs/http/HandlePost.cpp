#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

static unsigned long uploadCounter = 0;

std::string randomSessionId() {
	static unsigned long idCounter = 0;
	std::stringstream	 ss;
	ss << "id_" << std::time(0) << "_" << idCounter++;
	return ss.str();
}

void Response::handleLogin(const Request& request) {
	std::string body = request.getBody();
	std::string username;
	size_t		pos = body.find("username=");
	if (pos == std::string::npos) {
		setStatus(HTTP_302_FOUND);
		setHeader("Location", URI_WELCOME);
		return;
	}
	username = body.substr(pos + 9);
	if (username.empty()) {
		setStatus(HTTP_302_FOUND);
		setHeader("Location", URI_WELCOME);
		return;
	}
	bool		existUser = false;
	std::string sessionId;
	std::string cookie = request.getHeader("Cookie");
	if (!cookie.empty()) {
		size_t pos = cookie.find("session_id=");
		if (pos != std::string::npos) {
			sessionId = cookie.substr(pos + 11);
			std::map<std::string, std::string>::iterator it;
			for (it = _sessions->begin(); it != _sessions->end(); it++) {
				if (it->first == sessionId) {
					it->second = username;
					existUser			= true;
					break;
				}
			}
		}
	}
	if (existUser == false) {
		sessionId = randomSessionId();
		(*_sessions)[sessionId] = username;
		setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly; Max-Age=60");
	}
	setStatus(HTTP_302_FOUND);
	setHeader("Location", URI_DASHBOARD);
	return;
}

void Response::multiPart(Request& request, const MultipartField& part, std::string uploadDir) {
	if (part.filename.empty()) {
		request.setFormData(part.name, part.data);
		setStatus(HTTP_201_CREATED);
		setHeader("Content-Type", "text/plain");
		setBody("");
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

	// CGI check FIRST — before upload guard
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

	if (request.getUri() == "/welcome") {
		return (handleLogin(request));
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
	if (type.find("multipart/form-data") != std::string::npos) {
		std::string bodyStr = "Form data received successfully.\n";
		for (size_t i = 0; i < request.getMultipartFields().size(); i++) {
			multiPart(request, request.getMultipartFields()[i], uploadDir);
			if (getStatusCode() != HTTP_201_CREATED)
				return;
			bodyStr += getBody();
		}
		setBody(bodyStr + "\n");
	} else if (type == "application/x-www-form-urlencoded") {
		setStatus(HTTP_200_OK);
		setHeader("Content-Type", "text/plain");
		setBody("Form data received successfully.\n");
	} else {
		std::string uploadDir = locConf->upload_path;
		if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
			uploadDir += '/';

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
	// for debuging
	// std::map<std::string, std::string> myMap = request.getFormKeyValue();
	// for (std::map<std::string, std::string>::iterator it = myMap.begin();
	// 	it != myMap.end(); ++it)
	// {
	// 	std::cout << "||||||||||||||||||||||| key:" << it->first << " value:" << it->second <<
	// std::endl;
	// }
}
