#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include "../Head.hpp"

class Mime {
	private:
	typedef std::map<std::string, std::string>::iterator MimeIt;
	static std::map<std::string, std::string> _types;
	static std::map<std::string, std::string> _initTypes();

	public:
	static std::string getType(const std::string& extension);
	static std::string getExtension(const std::string& mimeType);

	private:
	Mime();
};
#endif
