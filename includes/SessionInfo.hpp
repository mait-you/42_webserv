#ifndef SESSION_HPP
#define SESSION_HPP

#include "Head.hpp"

struct SessionInfo {
	typedef std::map<std::string, SessionInfo> Map;
	bool isLogged;
	std::string username;
};

#endif
