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

void Response::handleFile(const Request& request, const std::string& fullPath) {
	const LocationConfig* locConf = request.getLocationConf();
	std::ifstream		  file(fullPath.c_str());
	if (!file) {
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
	setHeader("Content-type", Mime::getType(getExtension(fullPath)));
	setBody(ss.str());
}

void Response::handleDir(const Request& request, const std::string& fullPath) {
	const LocationConfig* locConf = request.getLocationConf();
	// const std::string&	  uri	  = request.getUri();
	const std::string&	  uri	  = request.getresolveUri(); // ! i add this

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

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(request, HTTP_201_CREATED);
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
