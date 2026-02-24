
#include "../../includes/BuildResponse.hpp"
#include "../../includes/MimeTypes.hpp"

Response handleFile(ServerConfig &srv, LocationConfig *locConfig, const std::string &fullPath);

bool allowedMethods(LocationConfig *locConfig, Request &req)
{
	for (size_t i = 0; i < locConfig->allow_methods.size(); i++)
	{
		if (req.getMethod() == locConfig->allow_methods[i])
			return true;
	}
	return false;
}

bool bodySize(ServerConfig &srv, Request &req)
{
	std::string maxStr = req.getHeader("Content-Length");
	if (maxStr.empty())
		return true;
	unsigned long bodyLen = 0;
	std::stringstream ss(maxStr);
	ss >> bodyLen;
	if (ss.fail() || !ss.eof())
		return false;
	if (bodyLen <= srv.client_max_body_size)
		return true;
	return false;
}

std::string getExtension(std::string fullPath)
{
	size_t lastSlash = fullPath.find_last_of('/');
	if (lastSlash != std::string::npos)
		fullPath = fullPath.substr(lastSlash + 1);
	size_t pos = fullPath.find_last_of('.');
	if (pos != std::string::npos)
		return fullPath.substr(pos + 1);
	return "";
}

Response errorPage(ServerConfig &srv, LocationConfig *locConfig, int code, std::string codeMsg)
{
	Response res;
	if (locConfig->error_pages.find(code) != locConfig->error_pages.end())
		res = handleFile(srv , locConfig, locConfig->error_pages[code]);
	else if (srv.error_pages.find(code) != srv.error_pages.end())
		res = handleFile(srv , locConfig, srv.error_pages[code]);
	else
	{
		res.setHeader("Content-type", "text/html");
		std::string defaultErr = "<html><body style='display: flex; justify-content: center;''><h1>code";
		defaultErr += codeMsg;
		defaultErr += "</h1></body></html>";
		res.setBody(defaultErr);
	}
	res.setStatus(code, codeMsg);
	return res;
}

Response handleFile(ServerConfig &srv, LocationConfig *locConfig, const std::string &fullPath)
{
	Response res;
	std::ifstream file(fullPath.c_str());
	if (!file.is_open())
	{
		res = errorPage(srv, locConfig, 403 , "Forbidden");
		return res;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	std::string fileContent = ss.str();
	res.setStatus(200, "OK");
	std::string extension = getExtension(fullPath);
	Mime mm;
	res.setHeader("Content-type",mm.getType(extension));
	res.setBody(fileContent);
	return res;
}

std::string  getList(std::string fullPath, std::string uri)
{
	std::string  res;
	DIR *dir;

	dir = opendir(fullPath.c_str());
	if (dir == NULL)
	{
		return res;
	}
	struct dirent *entry;
	res += "<html><body><h1> Index of ";
	res += uri;
	res += "</h1> ";
	res += "<hr><pre style='display: flex; flex-direction: column; gap: 10px;'>";
	struct stat st;
	while ((entry = readdir(dir)) != NULL)
	{
		res += "<a href='";
		if (stat((fullPath + entry->d_name).c_str(), &st) == 0 && S_ISDIR(st.st_mode))
		{
			res += entry->d_name;
			res += "/";
			res += "'> ";
			res += entry->d_name;
			res += "/";
		}
		else
		{
			res += entry->d_name;
			res += "'> ";
			res += entry->d_name;
		}
		res += "</a>";
	}
	res += "</pre></body></html>";
	closedir(dir);
	return res;
}


Response handleDir(Request &req, ServerConfig &srv, LocationConfig *locConfig, const std::string &fullPath)
{
	Response res;
	std::string uri = req.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/')
	{
		res.setStatus(301, "Moved Permanently");
		res.setHeader("Location", uri + "/");
		return res;
	}
	std::string index;
	if (!locConfig->index.empty())
		index = locConfig->index;
	else
		index = srv.index;
	if (!index.empty())
	{
		std::string indexPath = fullPath + index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
		{
			res = handleFile(srv , locConfig, indexPath);
			return res;
		}
	}
	if (locConfig->autoindex)
	{
		std::string DirList = getList(fullPath, uri);
		if (!DirList.empty())
		{
			res.setStatus(200, "OK");
			res.setHeader("Content-type", "text/html");
			res.setBody(DirList);
		}
		else
			res = errorPage(srv, locConfig, 403, "Forbidden");
	}
	else
		res = errorPage(srv, locConfig, 403, "Forbidden");
	return res;
}

Response handleGet(Request &req, ServerConfig &srv, LocationConfig *locConfig)
{
	Response res;
	std::string root;
	if (!locConfig->root.empty())
		root = locConfig->root;
	else
		root = srv.root;
	std::string fullPath = root + req.getUri();
	struct stat buffer;
	if (stat(fullPath.c_str(), &buffer) != 0)
	{
		res = errorPage(srv, locConfig, 404 , "Not Found");
		return res;
	}
	if (S_ISREG(buffer.st_mode))
		res = handleFile(srv , locConfig, fullPath);
	else if (S_ISDIR(buffer.st_mode))
		res = handleDir(req, srv, locConfig, fullPath);
	else
		res = errorPage(srv, locConfig, 404 , "Not Found");
	return res;
}


ServerConfig matchedServer(Request &req, const std::vector<ServerConfig> &servers) {
	std::string			port = "8080";
	std::string			server_name;
	std::string			host		= req.getHeader("Host");
	const ServerConfig *matchedPort = NULL;

	size_t pos = host.find(':');
	if (pos != std::string::npos) {
		server_name = host.substr(0, pos);
		port		= host.substr(pos + 1);
	}

	for (size_t i = 0; i < servers.size(); i++) {
		for (size_t j = 0; j < servers[i].ports.size(); j++) {
			if (servers[i].ports[j] == port) {
				if (!matchedPort)
					matchedPort = &servers[i];
				if (servers[i].server_name == server_name)
					return servers[i];
			}
		}
	}
	if (matchedPort)
		return *matchedPort;
	return servers[0];
}

LocationConfig *matchedLocation(ServerConfig &srv, Request &req) {
	LocationConfig	  *matched	  = NULL;
	const std::string &uri		  = req.getUri();
	size_t			   matchedLen = 0;

	for (size_t i = 0; i < srv.locations.size(); i++) {
		std::string &path = srv.locations[i].path;
		if (uri.compare(0, path.size(), path) == 0) {
			if (path.size() > matchedLen) {
				matchedLen = path.size();
				matched	   = &srv.locations[i];
			}
		}
	}
	return matched;
}

Response buildResponse(Request &req, const std::vector<ServerConfig> &servers, Client &client)
{
	ServerConfig	srv		  = matchedServer(client.getRequest(), servers);
	LocationConfig *locConfig = matchedLocation(srv, client.getRequest());
	Response res;

	if (!req.isValid())
	{
		Request::HttpError error = req.getError();
		if (error == 400)
			res = errorPage(srv, locConfig, 400 , "Bad Request");
		else if(error == 505)
			res = errorPage(srv, locConfig, 505 , "HTTP Version Not Supported");
		return res;
	}
	if (!locConfig)
	{
		res = errorPage(srv, locConfig, 404 , "Not Found");
		return res;
	}
	if (locConfig->has_redirect)
	{
		if (locConfig->redirect_code == 301)
			res = errorPage(srv, locConfig, 301 , "Moved Permanently");
		else
			res = errorPage(srv, locConfig, 302 , "Found");
		res.setHeader("Location", locConfig->redirect_url);
		return res;
	}
	if (!allowedMethods(locConfig, req))
	{
		res = errorPage(srv, locConfig, 405 , "Method Not Allowed");
		return res;
	}
	if (!bodySize(srv, req))
	{
		res = errorPage(srv, locConfig, 413 , "Payload Too Large");
		return res;
	}

	if (req.getMethod() == "GET")
		res = handleGet(req, srv, locConfig);
	// else if (req.getMethod() == "POST")
	// 	 res = handlePost(req, srv, locConfig);
	// else if (req.getMethod() == "DELETE")
	// 	 res = handleDelete(req, srv, locConfig);

	return res;
}
