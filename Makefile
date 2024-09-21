# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -Iinclude

# Linker flags
LDFLAGS = -lncurses -lz -lbrotlienc -lbrotlidec

# Directories
SRC_DIR = src
BUILD_DIR = build

# Find all source files
SRC_FILES := $(shell find $(SRC_DIR) -name '*.c')

# Create corresponding object files paths
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Output executable (in the root directory)
TARGET = httproxy

# Default target
all: $(TARGET)

# Link the target
$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@) # Create the build directory if it does not exist
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
