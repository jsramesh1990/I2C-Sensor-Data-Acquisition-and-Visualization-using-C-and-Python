
### `Makefile`
```makefile
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include/backend
LDFLAGS = -lpthread -lsqlite3 -lm

# Directories
SRC_DIR = src/backend
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include/backend

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
TARGET = $(BIN_DIR)/sensor_backend

# Python GUI
GUI_DIR = src/gui
GUI_SCRIPT = $(GUI_DIR)/gui.py

# Default target
all: $(TARGET)

# Create directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Link object files
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) data/sensor_data.db

# Run tests
test: $(TARGET)
	cd tests && gcc -o test_backend test_backend.c ../$(TARGET) -I../$(INCLUDE_DIR) -lpthread -lsqlite3 -lm
	./tests/test_backend

# Install dependencies
deps:
	chmod +x scripts/install_deps.sh
	./scripts/install_deps.sh

# Run backend
run-backend: $(TARGET)
	./scripts/start_backend.sh

# Run GUI
run-gui:
	./scripts/start_gui.sh

# PHONY targets
.PHONY: all clean test deps run-backend run-gui
