#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

std::string extractId(std::string& cookie) {
	std::stringstream ss(cookie);
	std::string		  str;

	while (std::getline(ss, str, ';')) {
		size_t pos = str.find("session_id=");
		if (pos != std::string::npos && pos == 0)
			return str.substr(11);
	}
	return "";
}

void Response::handleDashboard(const Request& request) {
	std::stringstream ss;
	std::string		  cookie = request.getHeader("Cookie");
	if (!cookie.empty()) {
		std::string sessionId = extractId(cookie);
		if (!sessionId.empty()) {
			std::map<std::string, std::string>::iterator it;
			for (it = _sessions->begin(); it != _sessions->end(); it++) {
				if (it->first == sessionId) {
					ss << "<html> <head><title>Webserv - Dashboard</title>"
					   << "<link rel='stylesheet' href='css/style.css'></head> <body>"
					   << "<a href='/logout'>logout</a>"
					   << "<h1> Welcome, " << it->second << "</h1>"
					   << "<a href='/'>Back to Home</a></body></html>";
					setStatus(HTTP_200_OK);
					setHeader("Content-type", "text/html");
					setBody(ss.str());
					return;
				}
			}
		}
	}
	setStatus(HTTP_302_FOUND);
	setHeader("Location", URI_WELCOME);
}

void Response::handleLogout(const Request& request) {
	std::string cookie = request.getHeader("Cookie");
	if (!cookie.empty()) {
		std::string sessionId = extractId(cookie);
		if (!sessionId.empty()) {
			std::map<std::string, std::string>::iterator it;
			for (it = _sessions->begin(); it != _sessions->end(); it++) {
				if (it->first == sessionId) {
					_sessions->erase(sessionId);
					break;
				}
			}
		}
	}
	setHeader("Set-Cookie", "session_id=;Path=/; HttpOnly; Max-Age=0");
	setStatus(HTTP_302_FOUND);
	setHeader("Location", URI_WELCOME);
}

void Response::handleFile(const Request& request, const std::string& fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1) {
		errorPage(request, HTTP_404_NOT_FOUND);
		return;
	}
	const LocationConfig* locConf = request.getLocationConf();
	std::ifstream		  file(fullPath.c_str());
	if (!file.is_open()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}
	if (request.hasCgi()) {
		Cgi cgi(request, *request.getConf(), locConf, fullPath);
		_runningCgi = cgi.start();
		if (_runningCgi.pid == -1)
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		else
			_hasCgiRunning = true;
		return;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	setStatus(HTTP_200_OK);
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
}

void Response::handleDir(const Request& request, const std::string& fullPath) {
	const LocationConfig* locConf = request.getLocationConf();
	const std::string&	  uri	  = request.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/') {
		setStatus(HTTP_301_MOVED_PERMANENTLY);
		setHeader("Location", uri + "/");
		return;
	}

	std::string index;
	if (locConf)
		index = locConf->index;
	if (!index.empty()) {
		std::string indexPath = fullPath + index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
			handleFile(request, indexPath);
			return;
		}
	}
	if (request.hasCgi()) {
		Cgi cgi(request, *request.getConf(), locConf, fullPath);
		_runningCgi = cgi.start();
		if (_runningCgi.pid == -1)
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		else
			_hasCgiRunning = true;
	} else if (locConf->autoindex) {
		std::string dirList = getList(fullPath, request.getResolvePath());
		if (!dirList.empty()) {
			setStatus(HTTP_200_OK);
			setHeader("Content-type", "text/html");
			setBody(dirList);
		} else {
			errorPage(request, HTTP_403_FORBIDDEN);
		}
	} else {
		errorPage(request, HTTP_403_FORBIDDEN);
	}
}

void Response::handleGet(const Request& request) {
	std::string fullPath = request.getResolveFullPath();

	if (request.getUri() == URI_DASHBOARD)
		return (handleDashboard(request));
	else if (request.getUri() == URI_LOGOUT)
		return (handleLogout(request));

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(request, HTTP_404_NOT_FOUND);
		return;
	}

	if (S_ISREG(buffer.st_mode))
		handleFile(request, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		handleDir(request, fullPath);
	else {
		errorPage(request, HTTP_404_NOT_FOUND);
	}
}
