CC        := clang

BLD_DIR   := build
COMPCOM   := compile_commands.json

SRC_DIR  := ../src
INC_DIR  := ../inc

SRC   := $(wildcard $(SRC_DIR)/*.c)
SRC   := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))

OBJ   := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRC))
DEPS   := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.d,$(SRC))

USRC_DIR  := ../deps/Unity/src
USRC      := $(USRC_DIR)/unity.c
UOBJ      := $(BLD_DIR)/unity.o

CFLAGS    := -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -Wno-gnu-statement-expression-from-macro-expansion \
			 -std=c23 -I$(INC_DIR) -I$(SRC_DIR) -I$(USRC_DIR) \
			-DUNITY_OUTPUT_COLOR

DEPFLAGS  := -MMD -MP

TEST_BINS := test_lexer test_parser test_dyn_arr test_dyn_str test_shell

.PHONY: all
all: $(TEST_BINS)

test_lexer: $(UOBJ) $(OBJ) $(BLD_DIR)/test_lexer.o
	$(CC) $(CFLAGS) $^ -o $@

test_parser: $(UOBJ) $(OBJ) $(BLD_DIR)/test_parser.o
	$(CC) $(CLFAGS) $^ -o $@

test_dyn_arr: $(UOBJ) $(OBJ) $(BLD_DIR)/test_dyn_arr.o
	$(CC) $(CFLAGS) $^ -o $@

test_dyn_str: $(UOBJ) $(OBJ) $(BLD_DIR)/test_dyn_str.o
	$(CC) $(CFLAGS) $^ -o $@

test_shell: $(UOBJ) $(OBJ) $(BLD_DIR)/test_shell.o
	$(CC) $(CFLAGS) $^ -o $@

# Test module
$(BLD_DIR)/test_%.o: test_%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# C module
$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
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
