#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"

std::string randomSessionId() {
	static unsigned long idCounter = 0;
	std::stringstream	 ss;
	ss << "id_" << std::time(0) << "_" << idCounter++;
	return ss.str();
}

void Response::handleLogin(const Request& request)
{
	std::string body = request.getBody();
	std::string username;
	size_t		pos = body.find("username=");
	if (pos == std::string::npos) {
		setStatus(HTTP_302_FOUND);
		setHeader("Location", URI_LOGIN);
		return;
	}
	username = body.substr(pos + 9);
	if (username.empty()) {
		setStatus(HTTP_302_FOUND);
		setHeader("Location", URI_LOGIN);
		return;
	}
	bool		existUser = false;
	std::string sessionId;
	std::string cookie = request.getHeader("Cookie");
	if (!cookie.empty()) {
		size_t pos = cookie.find("session_id=");
		if (pos != std::string::npos) {
			sessionId = cookie.substr(pos + 11);
			std::map<std::string, SessionInfo>::iterator it;
			for (it = _sessions->begin(); it != _sessions->end(); it++) {
				if (it->first == sessionId) {
					it->second.isLogged = true;
					it->second.username = username;
					existUser			= true;
					break;
				}
			}
		}
	}
	if (existUser == false) {
		sessionId = randomSessionId();
		SessionInfo info;
		info.username			= username;
		info.isLogged			= true;
		(*_sessions)[sessionId] = info;
		setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	}
	setStatus(HTTP_302_FOUND);
	setHeader("Location", URI_DASHBOARD);
	return;
}


void handlepart()
{

}

void Response::multiPart(const Request& request, std::string type, std::string uploadDir)
{
	std::ofstream ff("./Ressources/lines.txt");
	std::string resHeader;
	std::string headrPart;
	std::string bodyPart;

	size_t boundaryPos = type.find("boundary=");
	if (boundaryPos == std::string::npos)
		return errorPage(request, HTTP_400_BAD_REQUEST);

	std::string boundary = "--" + type.substr(boundaryPos + 9);
	std::string boundaryEnd = boundary + "--";
	std::string fullBody = request.getBody();

	size_t sep = fullBody.find("\r\n\r\n");
	headrPart = fullBody.substr(0, sep);
	bodyPart = fullBody.substr(sep + 4);
	ff << "headrPart:";
	ff << headrPart;
	ff << "bodyPart:";
	ff << bodyPart;

	std::stringstream ssHeader(headrPart);
	std::string line;
	
	std::getline(ssHeader, line, '\r');
	if (line != boundary)
		return errorPage(request, HTTP_400_BAD_REQUEST);

	bool isFile = false;
	while (std::getline(ssHeader, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t namePos = line.find("filename=");
		if (namePos != std::string::npos)
			isFile = true;
		size_t typePos = line.find("Content-Type: ");
		if (typePos != std::string::npos)
		{
			resHeader = line.substr(typePos + 14);
			setHeader("Content-Type", resHeader);
			continue;
		}
	}
	
	if (!isFile)
	{
		std::cout << "i will handle www-urlencoded later" << std::endl;
		setStatus(HTTP_201_CREATED);
		setHeader("Content-Type", "text/plain");
		setBody("here we go");
		return;
	}

	std::ostringstream	 oss;
	static unsigned long uploadCounter = 0;
	oss << uploadDir << "upload_" << std::time(NULL) << "_" << uploadCounter++;

	std::string ext = Mime::getExtension(resHeader);

	if (!ext.empty())
		ext = "." + ext;

	const std::string filePath = oss.str() + ext;
	std::ofstream	  file(filePath.c_str(), std::ios::binary);
	if (!file) {
		errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
		return;
	}

	file.write(bodyPart.c_str(), bodyPart.size());
	file.close();

	setStatus(HTTP_201_CREATED);
	setHeader("Content-Type", "text/plain");
	setBody("File uploaded: " + filePath + "\n");
}
void Response::handlePost(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	// CGI check FIRST — before upload guard
	if (request.hasCgi()) {
		const std::string fullPath = request.getResolveFullPath();
		Cgi				  cgi(request, *request.getConf(), locConf, fullPath);
		CgiInfo			  info = cgi.start();
		if (info.pid == -1) {
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
			return;
		}
		_runningCgi	   = info;
		_hasCgiRunning = true;
		return;
	}

	if (request.getUri() == "/login") {
		return (handleLogin(request));
	}

	if (!locConf || !locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	std::string uploadDir = locConf->upload_path;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';

	std::string type = request.getHeader("content-type");
	if (type.find("multipart/form-data") != std::string::npos)
	{
		multiPart(request, type, uploadDir);
	}
	else
	{
		std::string uploadDir = locConf->upload_path;
		if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
			uploadDir += '/';

		std::ostringstream	 oss;
		static unsigned long uploadCounter = 0;
		oss << uploadDir << "upload_" << std::time(NULL) << "_" << uploadCounter++;

		std::string ext = Mime::getExtension(request.getHeader("Content-Type"));
		if (!ext.empty())
			ext = "." + ext;

		const std::string filePath = oss.str() + ext;
		std::ofstream	  file(filePath.c_str(), std::ios::binary);
		if (!file) {
			errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
			return;
		}

		const std::string& body = request.getBody();
		file.write(body.c_str(), body.size());
		file.close();
		setStatus(HTTP_201_CREATED);
		setHeader("Content-Type", "text/plain");
		setBody("File uploaded: " + filePath + "\n");
	}
}
