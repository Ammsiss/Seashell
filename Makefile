BIN      := seashell
CC       := clang
SRC_DIR  := src
INC_DIR  := inc
BLD_DIR  := build
COMPCOM  := compile_commands.json

CFLAGS   := -g -O0 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic \
			-Wno-gnu-statement-expression-from-macro-expansion \
			-std=c23 -I$(INC_DIR)
DEPFLAGS := -MMD -MP

SRCS   := $(wildcard $(SRC_DIR)/*.c)
OBJS   := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRCS))
DEPS   := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.d,$(SRCS))

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BLD_DIR) $(BIN) $(COMPCOM)

-include $(DEPS)
