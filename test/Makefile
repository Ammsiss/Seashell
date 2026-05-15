CC       := clang

MSRC_DIR  := ../src
MINC_DIR  := ../inc

USRC_DIR := ../deps/Unity/src

BLD_DIR  := build
COMPCOM  := compile_commands.json

CFLAGS   := -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -std=c23 -I$(MINC_DIR) -I$(USRC_DIR)
DEPFLAGS := -MMD -MP

USRC     := $(USRC_DIR)/unity.c
UOBJ     := $(BLD_DIR)/unity.o

.PHONY: all
all:
	$(MAKE) test_lexer

test_lexer: $(BLD_DIR)/lexer.o $(BLD_DIR)/test_lexer.o $(UOBJ)
	$(CC) $(CFLAGS) $^ -o $@

# C module
$(BLD_DIR)/%.o: $(MSRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Test module
$(BLD_DIR)/test_%.o: test_%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Unity module
$(UOBJ): $(USRC)
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BLD_DIR) $(COMPCOM)

-include $(BLD_DIR)/*.d
