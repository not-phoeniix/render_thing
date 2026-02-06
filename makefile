# compiler and flags (for compiling and linking)
CXX := g++
PRE_FLAGS := -fPIC -g -m64 -Wall -std=c++20
POST_FLAGS := -lglfw -lvulkan -ldl -shared
ARCHIVER := ar
ARCHIVE_FLAGS := rcs

# directories
SRC_DIR := src
LIB_DIR := include
BIN_DIR := bin
OBJ_DIR := $(BIN_DIR)/obj

# files
SRC := $(shell find $(SRC_DIR)/ -type f -iname "*.cpp")
OBJ := $(subst $(SRC_DIR),$(OBJ_DIR),$(foreach file,$(basename $(SRC)),$(file).o))
OBJ_COMBINED := $(OBJ_DIR)/render_thing.o
BIN_STATIC := $(BIN_DIR)/librender_thing.a
BIN_DYNAMIC := $(BIN_DIR)/librender_thing.so

# === build tasks =========================================

all: $(BIN_STATIC) $(BIN_DYNAMIC)

static: $(BIN_STATIC)	
	
dynamic: $(BIN_DYNAMIC)

$(BIN_DYNAMIC): $(OBJ)
	@printf "linking dynamic...  \t"
	@$(CXX) $(OBJ) $(POST_FLAGS) -o $(BIN_DYNAMIC)
	@printf "done :D\n"	

$(BIN_STATIC): $(OBJ_COMBINED)
	@printf "linking static...  \t"
	@$(ARCHIVER) $(ARCHIVE_FLAGS) $(BIN_STATIC) $(OBJ_COMBINED)
	@printf "done :D\n"	

$(OBJ_COMBINED): $(OBJ)
	@$(CXX) $(OBJ) $(POST_FLAGS) -o $(OBJ_COMBINED)

.SECONDEXPANSION:

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(dir $$@)
	@echo "compiling $<..."
	@$(CXX) -c $< $(PRE_FLAGS) -I $(LIB_DIR) -o $@

# ensure directories are created via custom task
%/:
	@mkdir -p $@

.PRECIOUS: %/

# === utility tasks =======================================

.PHONY: clean run setup

clean:
	@echo "cleaning project..."
	rm -rf $(OBJ_DIR) 
	rm -rf $(BIN_DIR) 
	@echo "project cleaned!"

# make dirs and create main file
setup:
	@echo "setting up project..."

	@echo "creating directories..."
	mkdir -p $(SRC_DIR) $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR)

	@echo "creating main.cpp..."
	@echo "#include <iostream>" >> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "int main() {" >> $(SRC_DIR)/main.cpp
	@echo "	std::cout << \"hello world!!\n\";">> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "	return 0;" >> $(SRC_DIR)/main.cpp
	@echo "}" >> $(SRC_DIR)/main.cpp

	@echo "set up project!!"
