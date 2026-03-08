#include "../../includes/Response.hpp"

// ServerConfig Response::matchedServer(const Request& req, const std::vector<ServerConfig>&
// servers) { 	std::string			port = "8080"; 	std::string			server_name; 	std::string
// host		= req.getHeader("Host"); 	const ServerConfig* matchedPort = NULL;

// 	size_t pos = host.find(':');
// 	if (pos != std::string::npos) {
// 		server_name = host.substr(0, pos);
// 		port		= host.substr(pos + 1);
// 	}

// 	for (size_t i = 0; i < servers.size(); i++) {
// 		for (size_t j = 0; j < servers[i].ports.size(); j++) {
// 			if (servers[i].ports[j] == port) {
// 				if (!matchedPort)
// 					matchedPort = &servers[i];
// 				if (servers[i].server_name == server_name)
// 					return servers[i];
// 			}
// 		}
// 	}
// 	if (matchedPort)
// 		return *matchedPort;
// 	return servers[0];
// }

// const LocationConfig* Response::matchedLocation(const ServerConfig& srv, const Request& req) {
// 	const  LocationConfig*	   matched	  = NULL;
// 	const std::string& uri		  = cleanUri(req.getUri());
// 	size_t			   matchedLen = 0;

// 	for (size_t i = 0; i < srv.locations.size(); i++) {
// 		const std::string& path = srv.locations[i].path;
// 		if (uri.compare(0, path.size(), path) == 0) {
// 			if (path.size() > matchedLen) {
// 				matchedLen = path.size();
// 				matched	   = &srv.locations[i];
// 			}
// 		}
// 	}
// 	return matched;
// }

void Response::errorPage(const Request& request, codeStatus code) {
	const LocationConfig* locConf = request.getLocationConf();
	const ServerConfig*	  srvConf = request.getServerConf();

	setStatus(code);
	if (locConf) {
		std::map<int, std::string>::const_iterator it = locConf->error_pages.find(code);
		if (it != locConf->error_pages.end() && handleErrorFile(it->second))
			return;
	} else if (srvConf) {
		std::map<int, std::string>::const_iterator it = srvConf->error_pages.find(code);
		if (it != srvConf->error_pages.end() && handleErrorFile(it->second))
			return;
	}
	setHeader("Content-type", "text/html");
	std::string defaultErr = "<html><body style='display:flex;justify-content:center;'><h1>";
	defaultErr += defaultMessage(code);
	defaultErr += "</h1></body></html>";
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

bool Response::bodySize(const Request& request) {
	std::string maxStr = request.getHeader("Content-Length");
	if (maxStr.empty())
		return true;
	unsigned long	  bodyLen = 0;
	std::stringstream ss(maxStr);
	ss >> bodyLen;
	if (ss.fail() || !ss.eof())
		return false;
	return bodyLen <= request.getServerConf()->client_max_body_size;
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
		std::string name = entry->d_name;
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

