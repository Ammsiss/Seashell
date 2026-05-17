BIN      := shell-debug
CC       := clang
BLD_DIR  := build
COMPCOM  := compile_commands.json

MSRC_DIR  := ../src
MINC_DIR  := ../inc

CFLAGS   := -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -std=c23 -I$(MINC_DIR)
DEPFLAGS := -MMD -MP


.PHONY: all
all: $(BIN)

$(BIN): $(BLD_DIR)/shell_debug.o $(BLD_DIR)/lexer.o
	$(CC) $(CFLAGS) $^ -o $@

# Debug binary
$(BLD_DIR)/shell_debug.o: shell_debug.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# C modules
$(BLD_DIR)/%.o: $(MSRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BLD_DIR) $(COMPCOM)

-include $(BLD_DIR)/*.d
