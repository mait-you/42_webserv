#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"

std::string randomSessionId() {
	static unsigned long idCounter = 0;
	std::stringstream	 ss;
	ss << "id_" << std::time(0) << "_" << idCounter++;
	return ss.str();
}

void Response::handlePost(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	// CGI check FIRST — before upload guard
	if (request.hasCgi()) {
		const std::string fullPath = request.resolveFullPath();
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

	if (request.getUri() == "/login") {
		std::string body = request.getBody();
		std::string username;
		size_t		pos = body.find("username=");
		if (pos == std::string::npos) {
			setStatus(HTTP_302_FOUND, "Found");
			setHeader("Location", URI_LOGIN);
			return;
		}
		username = body.substr(pos + 9);
		if (username.empty()) {
			setStatus(HTTP_302_FOUND, "Found");
			setHeader("Location", URI_LOGIN);
			return;
		}
		bool		existUser = false;
		std::string sessionId;
		std::string cookie = request.getHeader("Cookie");
		if (!cookie.empty()) {
			size_t pos = cookie.find("session_id=");
			if (pos != std::string::npos) {
				sessionId = cookie.substr(pos + 11);
				std::map<std::string, SessionInfo>::iterator it;
				for (it = _sessions->begin(); it != _sessions->end(); it++) {
					if (it->first == sessionId) {
						it->second.isLogged = true;
						it->second.username = username;
						existUser			= true;
						break;
					}
				}
			}
		}
		if (existUser == false) {
			sessionId = randomSessionId();
			SessionInfo info;
			info.username			= username;
			info.isLogged			= true;
			(*_sessions)[sessionId] = info;
			setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
		}
		setStatus(HTTP_302_FOUND, "Found");
		setHeader("Location", URI_DASHBOARD);
		return;
	}

	if (!locConf || !locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	std::string uploadDir = locConf->upload_path;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';

	std::ostringstream	 oss;
	static unsigned long uploadCounter = 0;
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

	setStatus(HTTP_201_CREATED, "Created");
	setHeader("Content-Type", "text/plain");
	setBody("File uploaded: " + filePath + "\n");
}
