#include "../../includes/config/Config.hpp"

void skipComments(std::ifstream &configFile)
{
	char c;
	while (configFile.get(c))
	{
		if (c == '\n')
			break;
	}
}

struct Token readWord(std::ifstream &configFile, char &c)
{
	struct Token token;
	token.type = word;
	token.value = c;

	while (configFile.peek() != EOF &&
			!isspace(configFile.peek()) &&
			configFile.peek() != '{' &&
			configFile.peek() != '}' &&
			configFile.peek() != ';')
	{
		configFile.get(c);
		token.value += c;
	}
	return token;
}

std::vector<Token> tokenize(const std::string &filename)
{
	char c;
	struct Token token;
	std::vector<Token> tokens;

	std::ifstream configFile(filename.c_str());
	if (!configFile)
	{
		throw std::runtime_error("Error: cannot open config file");
	}
	while (configFile.get(c))
	{
		if (isspace(c))
			continue;
		if (c == '#')
		{
			skipComments(configFile);
			continue;
		}
		if (c == '{')
		{
			token.type = openBrace;
			token.value = "{";
			tokens.push_back(token);
			continue;
		}
		if (c == '}')
		{
			token.type = closeBrace;
			token.value = "}";
			tokens.push_back(token);
			continue;
		}
		if (c == ';')
		{
			token.type = semiColone;
			token.value = ";";
			tokens.push_back(token);
			continue;
		}
		tokens.push_back(readWord(configFile,c));
	}
	configFile.close();
	return tokens;
}
