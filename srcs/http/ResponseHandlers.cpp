#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

int Response::handleErrorFile(const std::string &fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1)
		return 0;
	std::ifstream file(fullPath.c_str());
	if (!file.is_open())
		return 0;
	std::stringstream ss;
	ss << file.rdbuf();
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
	return 1;
}

void Response::errorPage(ServerConfig &srv, LocationConfig *locConfig, codeStatus code) {
	setStatus(code);
	if (locConfig && locConfig->error_pages.find(code) != locConfig->error_pages.end()) {
		if (handleErrorFile(locConfig->error_pages[code]))
			return;
	} else if (srv.error_pages.find(code) != srv.error_pages.end()) {
		if (handleErrorFile(srv.error_pages[code]))
			return;
	}
	setHeader("Content-type", "text/html");
	std::string defaultErr = "<html><body style='display:flex;justify-content:center;'><h1>";
	defaultErr += defaultMessage(code);
	defaultErr += "</h1></body></html>";
	setBody(defaultErr);
}

void Response::handleFile(ServerConfig &srv, LocationConfig *locConfig,
						  const std::string &fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1) {
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
		return;
	}
	std::ifstream file(fullPath.c_str());
	if (!file.is_open()) {
		errorPage(srv, locConfig, HTTP_403_FORBIDDEN);
		return;
	}
	if (locConfig->has_cgi)
	{
		std::string ext = getExtension(fullPath);
		if (locConfig->cgi.find(ext) != locConfig->cgi.end())
		{
			Cgi cgi(*_currentRequest, srv, locConfig, fullPath);
			_runningCgi = cgi.start(_clientFd);
			if (_runningCgi.pid == -1)
				errorPage(srv, locConfig, HTTP_500_INTERNAL_SERVER_ERROR);
			else
				_hasCgiRunning = true;
			return;
		}
	}
	std::stringstream ss;
	ss << file.rdbuf();
	setStatus(HTTP_200_OK);
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
}

void Response::handleDir(Request &req, ServerConfig &srv, LocationConfig *locConfig,
						 const std::string &fullPath) {
	std::string uri = req.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/') {
		setStatus(HTTP_301_MOVED_PERMANENTLY);
		setHeader("Location", uri + "/");
		return;
	}

	std::string index;
	if (!locConfig->index.empty())
		index = locConfig->index;
	else
		index = srv.index;
	if (!index.empty()) {
		std::string indexPath = fullPath + index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
			handleFile(srv, locConfig, indexPath);
			return;
		}
	}

	if (locConfig->autoindex) {
		std::string dirList = getList(fullPath, cleanUri(req.getUri()));
		if (!dirList.empty()) {
			setStatus(HTTP_200_OK, "OK");
			setHeader("Content-type", "text/html");
			setBody(dirList);
		} else {
			errorPage(srv, locConfig, HTTP_403_FORBIDDEN);
		}
	} else {
		errorPage(srv, locConfig, HTTP_403_FORBIDDEN);
	}
}

void Response::handleGet(Request &req, ServerConfig &srv, LocationConfig *locConfig) {
	std::string root;
	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root = srv.root;
	std::string fullPath = root + cleanUri(req.getUri());

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
		return;
	}
	if (S_ISREG(buffer.st_mode))
		handleFile(srv, locConfig, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		handleDir(req, srv, locConfig, fullPath);
	else
		errorPage(srv, locConfig, HTTP_404_NOT_FOUND);
}
