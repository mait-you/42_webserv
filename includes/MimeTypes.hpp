#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include "Head.hpp"

class Mime {
  private:
	std::map<std::string, std::string> _types;

  public:
	Mime();
	~Mime();
	std::string getType(const std::string &extension);
};
#endif
