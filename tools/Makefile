CC        := clang

BIN      := pty_test
BLD_DIR  := build
COMP_CMDS  := compile_commands.json

SRC_DIR  := ../src
INC_DIR  := ../inc

SRCS   := pty_test.c $(wildcard $(SRC_DIR)/*.c)
SRCS   := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))

OBJS   := $(BLD_DIR)/pty_test.o $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRCS))
DEPS   := $(BLD_DIR)/pty_test.d $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.d,$(SRCS))

CFLAGS    := -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
			-Wpedantic -Wno-gnu-statement-expression-from-macro-expansion \
			 -std=c23 -I$(INC_DIR) -I$(SRC_DIR)

DEPFLAGS  := -MMD -MP

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEPFLAGS) $^ -o $@

$(BLD_DIR)/pty_test.o: pty_test.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY:
clean:
	rm -rf $(BIN) $(BLD_DIR) $(COMP_CMDS)
