#include "../../includes/Cgi.hpp"

Cgi::Cgi() {
	_loc = NULL;
}

Cgi::Cgi(const Request& req, const ServerConfig& srv, const LocationConfig* loc,
		 const std::string& path)
		: _req(req), _srv(srv), _loc(loc), _scriptPath(path) {}

Cgi::Cgi(const Cgi& other) {
	_req		= other._req;
	_srv		= other._srv;
	_loc		= other._loc;
	_scriptPath = other._scriptPath;
}

Cgi& Cgi::operator=(const Cgi& other) {
	if (this != &other) {
		_req		= other._req;
		_srv		= other._srv;
		_loc		= other._loc;
		_scriptPath = other._scriptPath;
	}
	return *this;
}

Cgi::~Cgi() {}


CgiInfo::CgiInfo() : pid(-1), clientFd(-1)
{
}

std::vector<std::string> Cgi::createEnv() const
{
	std::vector<std::string> envVec;

	std::string query;
	std::string uri = _req.getUri();
	size_t		pos = uri.find('?');
	if (pos != std::string::npos) {
		query = uri.substr(pos + 1);
		uri	  = uri.substr(0, pos);
	}

	envVec.push_back("REQUEST_METHOD=" + _req.getMethod());
	envVec.push_back("QUERY_STRING=" + query);
	envVec.push_back("CONTENT_TYPE=" + _req.getHeader("Content-Type"));
	envVec.push_back("CONTENT_LENGTH=" + _req.getHeader("Content-Length"));

	std::string extension;
	size_t		dotPos = _scriptPath.rfind('.');
	if (dotPos != std::string::npos)
		extension = _scriptPath.substr(dotPos);

	std::string pathInfo;
	size_t		extPos = uri.find(extension);
	if (!extension.empty() && extPos != std::string::npos) {
		size_t afterPos = extPos + extension.length();
		if (afterPos < uri.length())
			pathInfo = uri.substr(afterPos);
	}

	envVec.push_back("SCRIPT_NAME=" + uri);
	envVec.push_back("SCRIPT_FILENAME=" + _scriptPath);
	envVec.push_back("PATH_INFO=" + pathInfo);
	envVec.push_back("SERVER_NAME=" + _srv.host);
	envVec.push_back("SERVER_PORT=" + _srv.ports[0]);
	envVec.push_back("SERVER_PROTOCOL=" + _req.getVersion());
	envVec.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envVec.push_back("REDIRECT_STATUS=200");

	std::map<std::string, std::string> myHeaders = _req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = myHeaders.begin();
		 it != myHeaders.end(); ++it) {
		std::string key = it->first;
		if (key == "Content-Type" || key == "Content-Length")
			continue;
		for (size_t i = 0; i < key.length(); i++) {
			if (key[i] == '-')
				key[i] = '_';
			else
				key[i] = std::toupper(key[i]);
		}
		envVec.push_back("HTTP_" + key + "=" + it->second);
	}

	return envVec;
}

CgiInfo Cgi::start(int clientFd)
{
	std::string extension;
	size_t		pos = _scriptPath.rfind('.');
	if (pos != std::string::npos)
		extension = _scriptPath.substr(pos);

	std::map<std::string, std::string>::const_iterator it = _loc->cgi.find(extension);
	if (it == _loc->cgi.end())
		return CgiInfo();
	const std::string& cgiPath = it->second;

	char* argv[3];
	argv[0] = const_cast<char*>(cgiPath.c_str());
	argv[1] = const_cast<char*>(_scriptPath.c_str());
	argv[2] = NULL;

	size_t slashPos = _scriptPath.rfind('/');
	std::string scriptDir = _scriptPath.substr(0, slashPos);

	std::vector<std::string> envVec = createEnv();
	std::vector<char *> envp;
	for (size_t i = 0; i < envVec.size(); i++)
		envp.push_back(const_cast<char*>(envVec[i].c_str()));
	envp.push_back(NULL);

	static int counter = 0;
	std::ostringstream bodyOss, resOss;
	bodyOss << "/tmp/cgi_body_" << time(NULL) << "_" << counter;
	resOss << "/tmp/cgi_res_" << time(NULL) << "_" << counter;
	counter++;

	std::string bodyPath = bodyOss.str();
	std::string resPath = resOss.str();

	std::ofstream bodyFile(bodyPath.c_str());
	if (!bodyFile.is_open())
		return CgiInfo();
	bodyFile << _req.getBody();
	bodyFile.close();

	std::ofstream responseFile(resPath.c_str());
	if (!responseFile.is_open())
	{
		unlink(bodyPath.c_str());
		return CgiInfo();
	}
	responseFile.close();

	pid_t pid = fork();
	if (pid == -1)
	{
		unlink(bodyPath.c_str());
		unlink(resPath.c_str());
		return CgiInfo();
	}

	if (pid == 0)
	{
		int bodyFd = open(bodyPath.c_str(), O_RDONLY);
		if (bodyFd == -1)
			_exit(1);
		if (dup2(bodyFd, STDIN_FILENO) == -1)
			_exit(1);
		close(bodyFd);

		int resFd = open(resPath.c_str(), O_WRONLY);
		if (resFd == -1)
			_exit(1);
		if (dup2(resFd, STDOUT_FILENO) == -1)
			_exit(1);
		close(resFd);

		if (chdir(scriptDir.c_str()) == -1)
			_exit(1);

		execve(cgiPath.c_str(), argv, &envp[0]);
		_exit(1);
	}

	CgiInfo info;
	info.pid = pid;
	info.clientFd = clientFd;
	info.resPath = resPath;
	info.bodyPath = bodyPath;
	return info;
}
