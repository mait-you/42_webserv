#ifndef SESSION_HPP
#define SESSION_HPP

#include "../Head.hpp"

#define URI_LOGIN      "/login.html"
#define URI_DASHBOARD  "/dashboard.html"
#define URI_LOGOUT     "/logout.html"

struct SessionInfo {
	typedef std::map<std::string, SessionInfo> Map;
	bool isLogged;
	std::string username;
};

#endif
