CC        := clang

MSRC_DIR  := ../src
MINC_DIR  := ../inc

USRC_DIR  := ../deps/Unity/src

BLD_DIR   := build
COMPCOM   := compile_commands.json

CFLAGS    := -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -Wno-gnu-statement-expression-from-macro-expansion \
			 -std=c23 -I$(MINC_DIR) -I$(USRC_DIR) \
			-DUNITY_OUTPUT_COLOR
DEPFLAGS  := -MMD -MP

USRC      := $(USRC_DIR)/unity.c
UOBJ      := $(BLD_DIR)/unity.o

LEX_MODS  := $(UOBJ) $(BLD_DIR)/test_lexer.o $(BLD_DIR)/lexer.o \
			 $(BLD_DIR)/dyn_arr.o
PS_MODS   := $(UOBJ) $(BLD_DIR)/test_parser.o $(BLD_DIR)/parser.o \
			 $(BLD_DIR)/lexer.o $(BLD_DIR)/dyn_arr.o
DA_MODS   := $(UOBJ) $(BLD_DIR)/test_dyn_arr.o $(BLD_DIR)/dyn_arr.o
EX_MODS   := $(UOBJ) $(BLD_DIR)/test_expander.o $(BLD_DIR)/dyn_arr.o \
			 $(BLD_DIR)/parser.o $(BLD_DIR)/lexer.o $(BLD_DIR)/expander.o

TEST_BINS := test_lexer test_dyn_arr test_parser test_expander

.PHONY: all
all: $(TEST_BINS)

test_lexer: $(LEX_MODS)
	$(CC) $(CFLAGS) $^ -o $@

test_parser: $(PS_MODS)
	$(CC) $(CLFAGS) $^ -o $@

test_dyn_arr: $(DA_MODS)
	$(CC) $(CFLAGS) $^ -o $@

test_expander: $(EX_MODS)
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
	rm -rf $(BLD_DIR) $(COMPCOM) $(TEST_BINS)

-include $(BLD_DIR)/*.d
