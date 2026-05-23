CC       := clang

MSRC_DIR  := ../src
MINC_DIR  := ../inc

USRC_DIR := ../deps/Unity/src

BLD_DIR  := build
COMPCOM  := compile_commands.json

CFLAGS   := -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -std=c23 -I$(MINC_DIR) -I$(USRC_DIR)
DEPFLAGS := -MMD -MP

USRC     := $(USRC_DIR)/unity.c
UOBJ     := $(BLD_DIR)/unity.o

LEX_MODS := $(UOBJ) $(BLD_DIR)/test_lexer.o $(BLD_DIR)/lexer.o \
			$(BLD_DIR)/dyn_arr.o
DA_MODS  := $(UOBJ) $(BLD_DIR)/test_dyn_arr.o $(BLD_DIR)/dyn_arr.o

.PHONY: all
all: test_lexer test_dyn_arr

test_lexer: $(LEX_MODS)
	$(CC) $(CFLAGS) $^ -o $@

test_dyn_arr: $(DA_MODS)
	$(CC) $(CFLAGS) $^ -o $@

# Test module
$(BLD_DIR)/test_%.o: test_%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# C module
$(BLD_DIR)/%.o: $(MSRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Unity module
$(UOBJ): $(USRC)
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BLD_DIR) $(COMPCOM) $(T_LEXER) $(T_DYN_ARR)

-include $(BLD_DIR)/*.d
