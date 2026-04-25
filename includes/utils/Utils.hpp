#ifndef UTILS_HPP
#define UTILS_HPP

#include "../Head.hpp"

void		setupSignals();
bool		isNumber(const std::string& str);
bool		isValidPort(const std::string& str);
std::string ipv4Tostr(uint32_t ip);
std::string portTostr(uint16_t port);
std::string toLower(const std::string& s);
std::string trimStr(const std::string& s);
bool		getLine(const std::string& buf, std::size_t& pos, std::string& line);
std::string getExtension(const std::string& fullPath);

std::string toString(int val);
std::string htmlEscape(const std::string& s);

std::string resolvePath(const std::string& uri);

void printEscaped(const std::string& s);

#endif
