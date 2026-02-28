#include "../../includes/Response.hpp"

ServerConfig Response::matchedServer(Request						 &req,
									 const std::vector<ServerConfig> &servers) {
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

LocationConfig *Response::matchedLocation(ServerConfig &srv, Request &req) {
	LocationConfig	  *matched	  = NULL;
	const std::string &uri		  = cleanUri(req.getUri());
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

bool Response::allowedMethods(LocationConfig *locConfig, Request &req) {
	for (size_t i = 0; i < locConfig->allow_methods.size(); i++) {
		if (req.getMethod() == locConfig->allow_methods[i])
			return true;
	}
	return false;
}

bool Response::bodySize(ServerConfig &srv, Request &req) {
	std::string maxStr = req.getHeader("Content-Length");
	if (maxStr.empty())
		return true;
	unsigned long	  bodyLen = 0;
	std::stringstream ss(maxStr);
	ss >> bodyLen;
	if (ss.fail() || !ss.eof())
		return false;
	return bodyLen <= srv.client_max_body_size;
}

std::string Response::getExtension(const std::string &fullPath) {
	std::string name	  = fullPath;
	size_t		lastSlash = name.find_last_of('/');
	if (lastSlash != std::string::npos)
		name = name.substr(lastSlash + 1);
	size_t pos = name.find_last_of('.');
	if (pos != std::string::npos)
		return name.substr(pos + 1);
	return "";
}

std::string Response::getList(const std::string &fullPath,
							  const std::string &uri) {
	std::string res;
	DIR		   *dir = opendir(fullPath.c_str());
	if (!dir)
		return res;

	res += "<html><body><h1>Index of ";
	res += uri;
	res +=
		"</h1><hr><pre style='display:flex;flex-direction:column;gap:10px;'>";

	struct dirent *entry;
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

// localhost:8080/test/
std::string Response::cleanUri(std::string uri)
{
	std::string segment;
	std::vector<std::string> cleanPath;
	std::stringstream ss(uri);

	while (std::getline(ss, segment, '/'))
	{
		if (segment.empty() || segment == ".")
			continue;
		if (segment == "..")
		{
			if (!cleanPath.empty())
				cleanPath.pop_back();
		}
		else
		{
			cleanPath.push_back(segment);
		}
	}

	std::string buffer;
	for (size_t i = 0; i < cleanPath.size(); i++)
	{
		buffer += "/";
		buffer += cleanPath[i];
	}
	if (cleanPath.empty() || uri[uri.size() - 1] == '/')
		buffer += "/";
	
	return buffer;
}
