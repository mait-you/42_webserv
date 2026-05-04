#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"
#include "../../includes/utils/Utils.hpp"

void Response::errorPage(const Request& request, CodeStatus code) {
	if (_httpVersion == HTTP_0_9)
		return;
	const ServerConfig*						   srvConf = request.getConf();
	const LocationConfig*					   locConf = request.getLocationConf();
	std::string								   errorPagePath, root;
	std::map<int, std::string>::const_iterator it;

	setStatus(code);

	if (locConf) {
		std::cerr << "loc not null" << std::endl;
		it = locConf->error_pages.find(getStatusCode());
		if (it != locConf->error_pages.end())
			errorPagePath = it->second;
		root = locConf->root;
	} else if (srvConf) {
		std::cerr << "server not null" << std::endl;
		it = srvConf->error_pages.find(getStatusCode());
		if (it != srvConf->error_pages.end())
			errorPagePath = it->second;
		root = srvConf->root;
	}

	if ((locConf || srvConf ) && !errorPagePath.empty()) {
		std::string fullPath;
		std::cerr << "hna" << std::endl;
		if (root[root.length() - 1] == '/' && it->second[0] == '/')
			fullPath = root + it->second.substr(1);
		else if (root[root.length() - 1] != '/' && it->second[0] != '/')
			fullPath = root + "/" + errorPagePath;
		else
			fullPath = root + errorPagePath;
		if (handleErrorFile(fullPath))
			return;
	}

	setHeader("Content-type", "text/html");
	std::stringstream ss;
	ss << "<html><body style='display:flex;justify-content:center;'><h1>" << getStatusCode() << " "
	   << getStatusMessage() << "</h1></body></html>\n";
	setBody(ss.str());
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

int Response::handleErrorFile(const std::string& fullPath) {
	if (access(fullPath.c_str(), F_OK) == -1)
		return 0;
	std::ifstream file(fullPath.c_str());
	if (!file)
		return 0;
	std::stringstream ss;
	ss << file.rdbuf();
	std::string extension = getExtension(fullPath);
	setHeader("Content-type", Mime::getType(extension));
	setBody(ss.str());
	return 1;
}
