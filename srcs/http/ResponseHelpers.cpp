#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

void Response::errorPage(const Request& request, CodeStatus code) {
	if (_httpVersion == HTTP_0_9)
		return;
	const LocationConfig* locConf = request.getLocationConf();
	const ServerConfig*	  srvConf = request.getConf();

	setStatus(code);
	if (locConf) {
		std::map<int, std::string>::const_iterator it = locConf->error_pages.find(getStatusCode());
		if (it != locConf->error_pages.end() && handleErrorFile(it->second))
			return;
	}
	if (srvConf && locConf) {
		std::map<int, std::string>::const_iterator it = srvConf->error_pages.find(getStatusCode());

		if (it != srvConf->error_pages.end()) {
			std::string path;
			std::string root = !locConf->root.empty() ? locConf->root : srvConf->root;
			if (root[root.length() - 1] == '/' && it->second[0] == '/')
				path = root + it->second.substr(1);
			else if (root[root.length() - 1] != '/' && it->second[0] != '/')
				path = root + "/" + it->second;
			else
				path = root + it->second;
			if (handleErrorFile(path))
				return;
		}
	}
	setHeader("Content-type", "text/html");
	std::string defaultErr = "<html><body style='display:flex;justify-content:center;'><h1>";
	defaultErr += getStatusMessage();
	defaultErr += "</h1></body></html>\n";
	setBody(defaultErr);
}

bool Response::allowedMethods(const Request& request) {
	if (!request.getLocationConf())
		return false;
	for (size_t i = 0; i < request.getLocationConf()->allow_methods.size(); i++) {
		if (request.getMethod() == request.getLocationConf()->allow_methods[i])
			return true;
	}
	return false;
}

std::string Response::getList(const std::string& fullPath, const std::string& uri) {
	std::string res;
	DIR*		dir = opendir(fullPath.c_str());
	if (!dir)
		return res;

	res += "<html><body><h1>Index of ";
	res += uri;
	res += "</h1><hr><pre style='display:flex;flex-direction:column;gap:10px;'>";

	struct dirent* entry;
	struct stat	   st;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = htmlEscape(entry->d_name);
		if (name == "." || name == "..")
			continue;
		res += "<a href='";
		if (stat((fullPath + name).c_str(), &st) == 0 && S_ISDIR(st.st_mode))
			res += name + "/'>" + name + "/";
		else
			res += name + "'>" + name;
		res += "</a>";
	}
	res += "</pre></body></html>";
	closedir(dir);
	return res;
}
