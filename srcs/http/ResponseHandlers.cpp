#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

void Response::errorPage(ServerConfig &srv, LocationConfig *locConfig, int code,
						 const std::string &codeMsg) {
	if (locConfig && locConfig->error_pages.find(code) != locConfig->error_pages.end())
		handleFile(srv, locConfig, locConfig->error_pages[code]);
	else if (srv.error_pages.find(code) != srv.error_pages.end())
		handleFile(srv, locConfig, srv.error_pages[code]);
	else {
		setHeader("Content-type", "text/html");
		std::string defaultErr =
			"<html><body style='display:flex;justify-content:center;'><h1>";
		defaultErr += codeMsg;
		defaultErr += "</h1></body></html>";
		setBody(defaultErr);
	}
	setStatus(code, codeMsg);
}

void Response::handleFile(ServerConfig &srv, LocationConfig *locConfig,
						  const std::string &fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1) {
		errorPage(srv, locConfig, 404, "Not Found");
		return;
	}
	std::ifstream file(fullPath.c_str());
	if (!file.is_open()) {
		errorPage(srv, locConfig, 403, "Forbidden");
		return;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	setStatus(200, "OK");
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
}

void Response::handleDir(Request &req, ServerConfig &srv,
						 LocationConfig	   *locConfig,
						 const std::string &fullPath) {
	std::string uri = req.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/') {
		setStatus(301, "Moved Permanently");
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
			setStatus(200, "OK");
			setHeader("Content-type", "text/html");
			setBody(dirList);
		} else {
			errorPage(srv, locConfig, 403, "Forbidden");
		}
	} else {
		errorPage(srv, locConfig, 403, "Forbidden");
	}
}

void Response::handleGet(Request &req, ServerConfig &srv,
						 LocationConfig *locConfig) {
	std::string root;
	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root= srv.root;
	std::string fullPath = root + cleanUri(req.getUri());

	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0) {
		errorPage(srv, locConfig, 404, "Not Found");
		return;
	}
	if (S_ISREG(buffer.st_mode))
		handleFile(srv, locConfig, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		handleDir(req, srv, locConfig, fullPath);
	else
		errorPage(srv, locConfig, 404, "Not Found");
}
