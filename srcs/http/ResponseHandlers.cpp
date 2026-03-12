#include "../../includes/MimeTypes.hpp"
#include "../../includes/Response.hpp"

int Response::handleErrorFile(const std::string& fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1)
		return 0;
	std::ifstream file(fullPath.c_str());
	if (!file.is_open())
		return 0;
	std::stringstream ss;
	ss << file.rdbuf();
	// ! file.close(); // zt lik hadi
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
	return 1;
}

void Response::handleFile(const Request& request, const std::string& fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1) {
		errorPage(request, HTTP_404_NOT_FOUND);
		return;
	}
	std::ifstream file(fullPath.c_str());
	if (!file.is_open()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}
	const LocationConfig* locConfig = request.getLocationConf();
	if (locConfig && locConfig->has_cgi)
	{
		std::string ext = getExtension(fullPath);
		if (locConfig->cgi.find(ext) != locConfig->cgi.end())
		{
			Cgi cgi(*_currentRequest, *request.getServerConf(), locConfig, fullPath);
			_runningCgi = cgi.start(_clientFd);
			if (_runningCgi.pid == -1)
				errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
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

void Response::handleDir(const Request& request, const std::string& fullPath) {
	const LocationConfig* locConf = request.getLocationConf();
	const std::string&	  uri	  = request.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/') {
		setStatus(HTTP_301_MOVED_PERMANENTLY);
		setHeader("Location", uri + "/");
		return;
	}

	std::string index;
	if (locConf && !locConf->index.empty())
		index = locConf->index;
	else
		index = request.getServerConf()->index;
	if (!index.empty()) {
		std::string indexPath = fullPath + index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
			handleFile(request, indexPath);
			return;
		}
	}

	if (locConf->autoindex) {
		std::string dirList = getList(fullPath, cleanUri(request.getUri()));
		if (!dirList.empty()) {
			setStatus(HTTP_200_OK, "OK");
			setHeader("Content-type", "text/html");
			setBody(dirList);
		} else {
			errorPage(request, HTTP_403_FORBIDDEN);
		}
	} else {
		errorPage(request, HTTP_403_FORBIDDEN);
	}
}
