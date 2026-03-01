NAME        = webserv

# Directories
SRC_DIR     = srcs
OBJ_DIR     = obj
INC_DIR     = includes

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 #-fsanitize=address -g


# Headers
HEADERS     = $(INC_DIR)/WebServer.hpp \
	$(INC_DIR)/Client.hpp \
	$(INC_DIR)/Config.hpp \
	$(INC_DIR)/Head.hpp \
	$(INC_DIR)/MimeTypes.hpp \
	$(INC_DIR)/Request.hpp \
	$(INC_DIR)/Response.hpp \
	$(INC_DIR)/HttpStatus.hpp \
	$(INC_DIR)/Socket.hpp

SRCS        = $(SRC_DIR)/main.cpp \
	$(SRC_DIR)/server/WebServer.cpp \
	$(SRC_DIR)/server/Socket.cpp \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/config/tokenize.cpp \
	$(SRC_DIR)/config/parseLocation.cpp \
	$(SRC_DIR)/config/serverConfig.cpp \
	$(SRC_DIR)/client/Client.cpp \
	$(SRC_DIR)/http/Request.cpp \
	$(SRC_DIR)/http/RequestValidtion.cpp \
	$(SRC_DIR)/utils/Utils.cpp \
	$(SRC_DIR)/http/ResponseHelpers.cpp \
	$(SRC_DIR)/http/ResponseHandlers.cpp \
	$(SRC_DIR)/http/HandlePost.cpp \
	$(SRC_DIR)/http/Response.cpp \
	$(SRC_DIR)/http/MimeTypes.cpp \
	$(SRC_DIR)/http/HttpStatus.cpp \
	$(SRC_DIR)/http/handleDelete.cpp

OBJS        = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Colors
GREEN       = \033[0;32m
BLUE        = \033[0;34m
NC          = \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	echo "$(GREEN)✓ $(NAME) compiled successfully$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	echo "$(BLUE)Compiled: $<$(NC)"

clean:
	rm -rf $(OBJ_DIR)
	echo "$(GREEN)✓ Object files cleaned$(NC)"

fclean: clean
	rm -f $(NAME)
	echo "$(GREEN)✓ $(NAME) removed$(NC)"

re: fclean all

.PHONY: all clean fclean re
.SILENT:
.NOTPARALLEL: all $(NAME) $(OBJS)%.o clean fclean re
