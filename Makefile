CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Isrc

BIN_DIR := bin
BUILD_DIR := build
TEST_DIR := tests
TEST_BIN_DIR := $(BIN_DIR)/tests


# =========================
# Tests
# =========================

$(TEST_BIN_DIR)/ipc_tb: tests/unit/ipc_tb.c $(BIN_DIR)/intercontroller
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

ipc_tb: $(TEST_BIN_DIR)/ipc_tb
	./$(TEST_BIN_DIR)/ipc_tb

$(TEST_BIN_DIR)/pipe_tb: tests/unit/pipe_tb.c src/ipc/pipes.c
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

pipe_tb: $(TEST_BIN_DIR)/pipe_tb
	./$(TEST_BIN_DIR)/pipe_tb


# =========================
# Main programs
# =========================

KERNEL_SOURCES := \
	src/main.c \
	src/core/kernelsim.c \
	src/ipc/pipes.c \
	src/utils/queue.c

KERNEL_OBJECTS := $(KERNEL_SOURCES:src/%.c=$(BUILD_DIR)/%.o)

all: $(BIN_DIR)/kernel $(BIN_DIR)/child $(BIN_DIR)/intercontroller

$(BIN_DIR)/kernel: $(KERNEL_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/child: $(BUILD_DIR)/process/child.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/intercontroller: $(BUILD_DIR)/ipc/intercontroller.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# =========================
# Clean
# =========================

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean ipc_tb pipe_tb