CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11 -Iinclude

# Directories
BIN_DIR := bin
BUILD_DIR := build

# Targets
all: $(BIN_DIR)/kernel $(BIN_DIR)/child

# Rule for Kernel Executable
$(BIN_DIR)/kernel: $(BUILD_DIR)/kernel_main.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule for Child Executable
$(BIN_DIR)/child: $(BUILD_DIR)/child_main.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compile source files into object files
$(BUILD_DIR)/%.o: src/kernel/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/child/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
