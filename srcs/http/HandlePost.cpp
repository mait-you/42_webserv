#include "../../includes/http/MimeTypes.hpp"
#include "../../includes/http/Response.hpp"

std::string randomSessionId() {
	static unsigned long idCounter = 0;
	std::stringstream	 ss;
	ss << "id_" << std::time(0) << "_" << idCounter++;
	return ss.str();
}

static std::string getFieldValue(const Request& request, const std::string& fieldName) {
	const Request::MultipartFields& fields = request.getMultipartFields();

	/* multipart path: walk the parsed parts */
	for (std::size_t i = 0; i < fields.size(); ++i) {
		if (fields[i].name == fieldName)
			return fields[i].data;
	}

	/* urlencoded fallback: "fieldName=value&..." */
	const std::string& body	 = request.getBody();
	const std::string  token = fieldName + "=";
	std::size_t		   pos	 = body.find(token);
	if (pos == std::string::npos)
		return "";
	pos += token.size();
	std::size_t end = body.find('&', pos);
	if (end == std::string::npos)
		end = body.size();
	return body.substr(pos, end - pos);
}


static std::string writePart(const MultipartField& field, const std::string& uploadDir,
							 unsigned long counter) {
	std::string safeName;

	if (!field.filename.empty()) {
		std::size_t slash = field.filename.rfind('/');
		safeName = (slash != std::string::npos) ? field.filename.substr(slash + 1) : field.filename;
	}

	std::ostringstream oss;

	oss << uploadDir << std::time(NULL) << "_" << counter << "_";

	if (safeName.empty()) {

		std::string ext = Mime::getExtension(field.contentType);
		if (!ext.empty())
			ext = "." + ext;
		oss << "upload" << ext;
	} else
		oss << safeName;

	const std::string filePath = oss.str();
	std::ofstream	  file(filePath.c_str(), std::ios::binary);
	if (!file)
		return "";

	file.write(field.data.c_str(), field.data.size());
	file.close();
	return filePath;
}


void Response::handleLogin(const Request& request) {
	std::string username = getFieldValue(request, "username");

	if (username.empty()) {
		setStatus(HTTP_302_FOUND);
		setHeader("Location", URI_LOGIN);
		return;
	}

	bool		existUser = false;
	std::string sessionId;
	std::string cookie = request.getHeader("Cookie");

	if (!cookie.empty()) {
		std::size_t pos = cookie.find("session_id=");
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

	if (!existUser) {
		sessionId = randomSessionId();
		SessionInfo info;
		info.username			= username;
		info.isLogged			= true;
		(*_sessions)[sessionId] = info;
		setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	}

	setStatus(HTTP_302_FOUND);
	setHeader("Location", URI_DASHBOARD);
}


void Response::handlePost(const Request& request) {
	const LocationConfig* locConf = request.getLocationConf();
	if (!locConf) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

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
		handleLogin(request);
		return;
	}

	if (!locConf->upload || locConf->upload_path.empty()) {
		errorPage(request, HTTP_403_FORBIDDEN);
		return;
	}

	std::string uploadDir = locConf->upload_path;
	if (uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';

	const std::string contentType = request.getHeader("Content-Type");


	if (contentType.find("multipart/form-data") != std::string::npos) {
		const Request::MultipartFields& fields = request.getMultipartFields();
		if (fields.empty()) {
			errorPage(request, HTTP_400_BAD_REQUEST);
			return;
		}
		std::string			 responseBody;
		bool				 anyFileWritten = false;
		static unsigned long uploadCounter	= 0;

		for (std::size_t i = 0; i < fields.size(); ++i) {
			if (fields[i].filename.empty())
				continue;

			std::string written = writePart(fields[i], uploadDir, uploadCounter++);
			if (written.empty()) {
				errorPage(request, HTTP_500_INTERNAL_SERVER_ERROR);
				return;
			}
			responseBody += "File uploaded: " + written + "\n";
			anyFileWritten = true;
		}

		if (!anyFileWritten) {
			errorPage(request, HTTP_400_BAD_REQUEST);
			return;
		}

		setStatus(HTTP_201_CREATED);
		setHeader("Content-Type", "text/plain");
		setBody(responseBody);
		return;
	}

	std::string ext = Mime::getExtension(contentType);
	if (!ext.empty())
		ext = "." + ext;

	std::ostringstream	 oss;
	static unsigned long uploadCounter = 0;
	oss << uploadDir << "upload_" << std::time(NULL) << "_" << uploadCounter++;

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
