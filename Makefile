# ==============================================================================
# KernelSim Project Makefile
# ==============================================================================

# ==============================================================================
# Compiler and Flags
# ==============================================================================
CC      := gcc
CFLAGS  := -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Isrc

# ==============================================================================
# Directory Structure
# ==============================================================================
BIN_DIR      := bin
BUILD_DIR    := build
TEST_DIR     := tests
TEST_BIN_DIR := $(BIN_DIR)/tests

# ==============================================================================
# Source and Object Files (KernelSim)
# ==============================================================================
KERNEL_SOURCES := 			\
    src/main.c 				\
    src/core/kernelsim.c 	\
    src/ipc/pipes.c 		\
    src/utils/queue.c

KERNEL_OBJECTS := $(KERNEL_SOURCES:src/%.c=$(BUILD_DIR)/%.o)

# ==============================================================================
# Default Target
# ==============================================================================
.PHONY: all
all: $(BIN_DIR)/kernel $(BIN_DIR)/child $(BIN_DIR)/intercontroller

# ==============================================================================
# Main Build Rules
# ==============================================================================

# Compiles the main KernelSim binary
$(BIN_DIR)/kernel: $(KERNEL_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

# Compiles the Application Process binary (Child)
$(BIN_DIR)/child: $(BUILD_DIR)/process/child.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

# Compiles the Interrupt Controller emulator binary
$(BIN_DIR)/intercontroller: $(BUILD_DIR)/ipc/intercontroller.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

# Generic rule for object files compilation
$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================================================================
# Testing Suite
# ==============================================================================
.PHONY: test_intercontroller test_pipes test_queues test_child test_all

# Intercontroller Test
$(TEST_BIN_DIR)/test_intercontroller: tests/suites/test_intercontroller.c $(BIN_DIR)/intercontroller
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

test_intercontroller: $(TEST_BIN_DIR)/test_intercontroller
	./$<

# Pipes Test
$(TEST_BIN_DIR)/test_pipes: tests/suites/test_pipes.c src/ipc/pipes.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

test_pipes: $(TEST_BIN_DIR)/test_pipes
	./$<

# Queues Test
$(TEST_BIN_DIR)/test_queues: tests/suites/test_queues.c src/utils/queue.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

test_queues: $(TEST_BIN_DIR)/test_queues
	./$<

# Child Process Test
$(TEST_BIN_DIR)/test_child: tests/suites/test_child.c $(BIN_DIR)/child
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

test_child: $(TEST_BIN_DIR)/test_child
	./$<

# Runs all tests sequentially
test_all: test_queues test_pipes test_intercontroller test_child

# ==============================================================================
# Cleanup and Utilities
# ==============================================================================
.PHONY: clean help

# Removes all build artifacts and compiled binaries
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Displays available make commands
help:
	@echo "KernelSim Build System - Available commands:"
	@echo "  make                 - Builds all main binaries (kernel, child, intercontroller)"
	@echo "  make clean           - Removes all compiled files and directories"
	@echo "  make test_all        - Compiles and runs all test suites sequentially"
	@echo "  make test_<module>   - Runs a specific test (queues, pipes, intercontroller, child)"
