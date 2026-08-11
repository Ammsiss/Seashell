BIN := seashell
CC := clang
COMPCOM := compile_commands.json

SRC_DIR := src
DEP_DIR := deps

INCLUDE := inc $(shell find $(DEP_DIR) -type d)
INCLUDE := $(patsubst %,-I%,$(INCLUDE))

BLD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRCS))
LIBS := $(shell find $(DEP_DIR) -type f -name '*.a')
DEPS := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.d,$(SRCS))

CFLAGS := -g -O0 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic \
			-Wno-gnu-statement-expression-from-macro-expansion \
			-std=c23 $(INCLUDE)

DEPFLAGS := -MMD -MP

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS) #$(LIBS)
	$(CC) $(CFLAGS) $^ -o $@

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BLD_DIR) $(BIN) $(COMPCOM)

-include $(DEPS)
