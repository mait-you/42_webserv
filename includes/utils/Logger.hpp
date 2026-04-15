#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "../Head.hpp"

#define ERROR_LOG(context, detail) throwError(__FILE__, __LINE__, context, detail)
#define WARNING_LOG(context, detail) printWarning(__FILE__, __LINE__, context, detail)

void throwError(const std::string& file, int line, const std::string& context,
				const std::string& detail);
void printWarning(const std::string& file, int line, const std::string& context,
				  const std::string& detail);

#endif
