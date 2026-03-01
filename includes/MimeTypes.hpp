#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include "Head.hpp"

class Mime {
  private:
	typedef std::map<std::string, std::string> MimeMap;
	typedef MimeMap::iterator				   MimeIt;
	typedef MimeMap::const_iterator			   ConstMimeIt;

  private:
	static MimeMap _types;
	static MimeMap _initTypes();

  public:
	static std::string getType(const std::string& extension);
	static std::string getExtension(const std::string& mimeType);

  private:
	Mime();
	Mime(const Mime&);
	Mime& operator=(const Mime&);
	~Mime();
};
#endif
