#include "../../includes/Cgi.hpp"

Cgi::Cgi() : _req(), _srv(), _loc(NULL), _scriptPath(""), _resPath(""), _bodyPath("") {}

Cgi::Cgi(const Request& req, const ServerConfig& srv, const LocationConfig* loc,
		 const std::string& path)
		: _req(req), _srv(srv), _loc(loc), _scriptPath(path), _resPath(""), _bodyPath("") {}

Cgi::Cgi(const Cgi& other) {
	_req		= other._req;
	_srv		= other._srv;
	_loc		= other._loc;
	_scriptPath = other._scriptPath;
	_resPath	= other._resPath;
	_bodyPath	= other._bodyPath;
}

Cgi& Cgi::operator=(const Cgi& other) {
	if (this != &other) {
		_req		= other._req;
		_srv		= other._srv;
		_loc		= other._loc;
		_scriptPath = other._scriptPath;
		_resPath	= other._resPath;
		_bodyPath	= other._bodyPath;
	}
	return *this;
}

Cgi::~Cgi() {}

CgiInfo::CgiInfo() : pid(-1) {}

std::vector<std::string> Cgi::createEnv() const {
	std::vector<std::string> envVec;
	std::string root = !_loc->root.empty() ? _loc->root : _srv.root;
	std::string query;
	std::string uri = _req.getUri();

	envVec.push_back("REQUEST_URI=" + uri);
	size_t pos = uri.find('?');
	if (pos != std::string::npos) {
		query = uri.substr(pos + 1);
		uri	  = uri.substr(0, pos);
	}
	envVec.push_back("DOCUMENT_ROOT=" + root);
	// envVec.push_back("REMOTE_ADDR=" + _req.getClientIp());

	envVec.push_back("REQUEST_METHOD=" + _req.getMethod());
	envVec.push_back("QUERY_STRING=" + query);
	envVec.push_back("CONTENT_TYPE=" + _req.getHeader("content-type"));
	envVec.push_back("CONTENT_LENGTH=" + _req.getHeader("content-length"));

	std::string extension;
	size_t		dotPos = _scriptPath.rfind('.');
	if (dotPos != std::string::npos)
		extension = _scriptPath.substr(dotPos);

	std::string pathInfo;
	size_t		extPos = uri.find(extension);
	if (!extension.empty() && extPos != std::string::npos) {
		size_t afterPos = extPos + extension.length();
		if (afterPos < uri.length()) {
			pathInfo = uri.substr(afterPos);
			uri		 = uri.substr(0, afterPos);
		}
	}

	if (pathInfo.empty()) {
        pathInfo = uri;
    }
	envVec.push_back("SCRIPT_NAME=" + uri);
	envVec.push_back("SCRIPT_FILENAME=" + _scriptPath);
	if (!pathInfo.empty()) {
		envVec.push_back("PATH_INFO=" + pathInfo);
		envVec.push_back("PATH_TRANSLATED=" + root + pathInfo);
	}
	envVec.push_back("SERVER_NAME=" + _srv.host);
	envVec.push_back("SERVER_PORT=" + _srv.ports[0]);
	envVec.push_back("SERVER_PROTOCOL=" + _req.getVersion());
	envVec.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envVec.push_back("REDIRECT_STATUS=200");
	envVec.push_back("SERVER_SOFTWARE=webserv/1.0");

	std::map<std::string, std::string> myHeaders = _req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = myHeaders.begin();
		 it != myHeaders.end(); ++it) {
		std::string key = it->first;
		if (key == "content-type" || key == "content-length")
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

const std::string Cgi::findCgiPath() const {
	std::string extension;
	size_t		pos = _scriptPath.rfind('.');
	if (pos != std::string::npos)
		extension = _scriptPath.substr(pos + 1);
	else
		return "";
	std::map<std::string, std::string>::const_iterator it = _loc->cgi.find(extension);
	if (it == _loc->cgi.end()) {
		std::string dotExt = "." + extension;
		it				   = _loc->cgi.find(dotExt);
	}
	if (it == _loc->cgi.end())
		return "";
	return it->second;
}

int Cgi::createFiles() {
	static unsigned long cgiCounter = 0;
	std::ostringstream	 bodyOss, resOss;
	bodyOss << "/tmp/cgi_body_" << std::time(0) << "_" << cgiCounter;
	resOss << "/tmp/cgi_res_" << std::time(0) << "_" << cgiCounter;
	cgiCounter++;
	_bodyPath = bodyOss.str();
	_resPath  = resOss.str();
	std::ofstream bodyFile(_bodyPath.c_str());
	if (!bodyFile.is_open())
		return 1;
	bodyFile << _req.getBody();
	bodyFile.close();
	std::ofstream responseFile(_resPath.c_str());
	if (!responseFile.is_open()) {
		unlink(_bodyPath.c_str());
		return 1;
	}
	responseFile.close();
	return 0;
}

CgiInfo Cgi::start() {
	const std::string& cgiPath = findCgiPath();
	if (cgiPath.empty())
		return CgiInfo();

	char* argv[3];
	argv[0]				   = const_cast<char*>(cgiPath.c_str());
	std::size_t		slashPos   = _scriptPath.rfind('/');
	std::string scriptDir  = _scriptPath.substr(0, slashPos);
	std::string scriptName = _scriptPath.substr(slashPos + 1);
	argv[1]				   = const_cast<char*>(scriptName.c_str());
	argv[2]				   = NULL;

	std::vector<std::string> envVec = createEnv();
	std::vector<char*>		 envp;
	for (std::size_t i = 0; i < envVec.size(); i++)
		envp.push_back(const_cast<char*>(envVec[i].c_str()));
	envp.push_back(NULL);

	if (createFiles() == 1)
		return CgiInfo();

	pid_t pid = fork();
	if (pid == -1) {
		unlink(_bodyPath.c_str());
		unlink(_resPath.c_str());
		return CgiInfo();
	}
	if (pid == 0) {
		int bodyFd = open(_bodyPath.c_str(), O_RDONLY);
		if (bodyFd == -1)
			_exit(1);
		if (dup2(bodyFd, STDIN_FILENO) == -1)
			_exit(1);
		close(bodyFd);
		int resFd = open(_resPath.c_str(), O_WRONLY);
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
	info.pid	   = pid;
	info.resPath   = _resPath;
	info.bodyPath  = _bodyPath;
	info.startTime = std::time(0);
	return info;
}
