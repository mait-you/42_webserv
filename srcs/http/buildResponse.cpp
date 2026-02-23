
#include "../../includes/BuildResponse.hpp"

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

Response handleFile(const std::string &fullPath)
{
	Response res;
	std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		res.setStatus(403, "Forbidden");
		return res;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	std::string fileContent = ss.str();
	res.setBody(fileContent);
	res.setStatus(200, "OK");
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
			res = handleFile(indexPath);
			return res;
		}
	}
	if (locConfig->autoindex)
	{
		res.setStatus(200, "OK");
		res.setBody("<html><body><h1>files inside this directory</h1></body></html>");
		return res;
	}
	res.setStatus(403, "Forbidden");
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
		res.setStatus(404, "Not Found");
		return res;
	}
	if (S_ISREG(buffer.st_mode))
		res = handleFile(fullPath);
	else if (S_ISDIR(buffer.st_mode))
		res = handleDir(req, srv, locConfig, fullPath);
	else
		res.setStatus(404, "Not Found");
	return res;
}

Response buildResponse(Request &req, ServerConfig &srv, LocationConfig *locConfig)
{
	Response res;

	if (!locConfig)
	{
		res.setStatus(404, "Not Found");
		return res;
	}
	if (locConfig->has_redirect)
	{
		if (locConfig->redirect_code == 301)
			res.setStatus(locConfig->redirect_code,"Moved Permanently");
		else
			res.setStatus(locConfig->redirect_code,"Found");
		res.setHeader("Location", locConfig->redirect_url);
		return res;
	}
	if (!allowedMethods(locConfig, req))
	{
		res.setStatus(405, "Method Not Allowed");
		return res;
	}
	if (!bodySize(srv, req))
	{
		res.setStatus(413, "Payload Too Large");
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
