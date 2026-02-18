NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

# Directories
SRC_DIR     = srcs
OBJ_DIR     = obj
INC_DIR     = includes

# Source files
SRCS        = $(SRC_DIR)/main.cpp \
	$(SRC_DIR)/server/WebServer.cpp \
	$(SRC_DIR)/server/Socket.cpp \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/client/Client.cpp \
	$(SRC_DIR)/http/Request.cpp \
	$(SRC_DIR)/utils/Utils.cpp

OBJS        = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Colors
GREEN       = \033[0;32m
BLUE        = \033[0;34m
NC          = \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	echo "$(GREEN)✓ $(NAME) compiled successfully$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
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
