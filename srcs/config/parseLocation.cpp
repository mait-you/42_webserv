#include "../../includes/Config.hpp"

void parseIndex(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected auto index");
	}
	location.upload_path = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseUploadPath(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected upload path");
	}
	location.upload_path = tokens[i].value;
	location.upload		 = true;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseAutoIndex(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected auto index");
	}
	if (tokens[i].value == "on") {
		location.autoindex = true;
	} else if (tokens[i].value == "off") {
		location.autoindex = false;
	} else {
		throw std::runtime_error("Invalid config: Expected auto index");
	}
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseAllowedMethods(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected upload allowed methods");
	}
	location.allow_methods.clear();
	for (int j = 1; j <= 3; j++) {
		if (tokens[i].type == word && (tokens[i].value == "GET" || tokens[i].value == "POST" ||
									   tokens[i].value == "DELETE")) {
			location.allow_methods.push_back(tokens[i].value);
			i++;
		} else if (tokens[i].type == semiColone && j != 1) {
			break;
		} else {
			// i think should return error codeStatus ? when method not found ?
			throw std::runtime_error("Invalid config: unexpected upload method");
		}
	}
	if (tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseErrorPage(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	// add this later: muti errors codes can share same error page
	// error_page HTTP_500_INTERNAL_SERVER_ERROR 502 503 /50x.html;
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected error page");
	}
	unsigned int	  errorCode;
	std::stringstream ss(tokens[i].value);
	ss >> errorCode;
	if (ss.fail() || !ss.eof()) {
		throw std::runtime_error("Invalid config: Expected error codeStatus");
	}
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected error page path");
	}
	location.error_pages[errorCode] = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseCgiPass(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected upload path");
	}
	location.cgi_path	   = tokens[i].value;
	location.has_cgi	   = true;
	location.cgi_extension = location.path;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseredirection(size_t &i, std::vector<Token> &tokens, LocationConfig &location) {
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected redirection status codeStatus");
	}
	unsigned int	  statusCode;
	std::stringstream ss(tokens[i].value);
	ss >> statusCode;
	if (ss.fail() || !ss.eof()) {
		throw std::runtime_error("Invalid config: Expected redirection status codeStatus");
	}
	if (statusCode != 301 && statusCode != 302) {
		throw std::runtime_error("Invalid config: status codeStatus not allowed");
	}
	i++;
	if (i >= tokens.size() || tokens[i].type != word) {
		throw std::runtime_error("Invalid config: Expected redirection url");
	}
	if (location.has_redirect == true) {
		throw std::runtime_error("Invalid config: must be one redirection per location");
	}
	location.has_redirect  = true;
	location.redirect_code = statusCode;
	location.redirect_url  = tokens[i].value;
	i++;
	if (i >= tokens.size() || tokens[i].type != semiColone) {
		throw std::runtime_error("Invalid config: Expected ;");
	}
	i++;
}

void parseLocation(std::vector<Token> &tokens, size_t &i, LocationConfig &location) {
	size_t tokenSize = tokens.size();
	if (i >= tokenSize) {
		throw std::runtime_error("Invalid config: expected }");
	}
	while (i < tokenSize) {
		if (tokens[i].type == closeBrace) {
			i++;
			return;
		} else if (tokens[i].type == word && tokens[i].value == "index")
			parseIndex(i, tokens, location);
		else if (tokens[i].type == word && tokens[i].value == "upload_path")
			parseUploadPath(i, tokens, location);
		else if (tokens[i].type == word && tokens[i].value == "autoindex")
			parseAutoIndex(i, tokens, location);
		else if (tokens[i].type == word && tokens[i].value == "allow_methods")
			parseAllowedMethods(i, tokens, location);
		else if (tokens[i].type == word && tokens[i].value == "error_page")
			parseErrorPage(i, tokens, location);

		else if (tokens[i].type == word && tokens[i].value == "cgi_pass")
			parseCgiPass(i, tokens, location);
		else if (tokens[i].type == word && tokens[i].value == "return")
			parseredirection(i, tokens, location);
		else {
			std::string str = "Invalid config: unexpected token '" + tokens[i].value + "'";
			throw std::runtime_error(str);
		}
	}
}
