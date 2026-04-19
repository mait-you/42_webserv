NAME    = webserv
SRC_DIR = srcs
OBJ_DIR = obj
INC_DIR = includes

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

HEADERS = \
	$(INC_DIR)/Head.hpp                     \
	$(INC_DIR)/net/WebServer.hpp            \
	$(INC_DIR)/net/Socket.hpp               \
	$(INC_DIR)/net/Client.hpp               \
	$(INC_DIR)/cgi/Cgi.hpp                  \
	$(INC_DIR)/config/Config.hpp            \
	$(INC_DIR)/http/MimeTypes.hpp           \
	$(INC_DIR)/http/Request.hpp             \
	$(INC_DIR)/http/Response.hpp            \
	$(INC_DIR)/http/HttpStatus.hpp          \
	$(INC_DIR)/session/SessionInfo.hpp      \
	$(INC_DIR)/utils/Logger.hpp             \
	$(INC_DIR)/utils/Utils.hpp

SRCS = \
	$(SRC_DIR)/main.cpp                     \
	$(SRC_DIR)/net/WebServer.cpp            \
	$(SRC_DIR)/net/Socket.cpp               \
	$(SRC_DIR)/net/Client.cpp               \
	$(SRC_DIR)/config/Config.cpp            \
	$(SRC_DIR)/config/Tokenize.cpp          \
	$(SRC_DIR)/config/ParseLocation.cpp     \
	$(SRC_DIR)/config/ServerConfig.cpp      \
	$(SRC_DIR)/http/Request.cpp             \
	$(SRC_DIR)/http/RequestValidtion.cpp    \
	$(SRC_DIR)/http/ResponseHelpers.cpp     \
	$(SRC_DIR)/http/ResponseHandlers.cpp    \
	$(SRC_DIR)/http/HandlePost.cpp          \
	$(SRC_DIR)/http/HandleGet.cpp           \
	$(SRC_DIR)/http/HandleDelete.cpp        \
	$(SRC_DIR)/http/Response.cpp            \
	$(SRC_DIR)/http/MimeTypes.cpp           \
	$(SRC_DIR)/http/HttpStatus.cpp          \
	$(SRC_DIR)/cgi/Cgi.cpp                  \
	$(SRC_DIR)/utils/Logger.cpp             \
	$(SRC_DIR)/utils/Utils.cpp

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

GREEN = \033[0;32m
BLUE  = \033[0;34m
NC    = \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	echo "$(GREEN)✓ $(NAME) compiled$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	echo "$(BLUE)  compiled: $<$(NC)"

clean:
	$(RM) -r $(OBJ_DIR)
	echo "$(GREEN)✓ objects removed$(NC)"

fclean: clean
	$(RM) $(NAME)
	echo "$(GREEN)✓ $(NAME) removed$(NC)"

re: fclean all

.PHONY: all clean fclean re
.SILENT:
.NOTPARALLEL:
