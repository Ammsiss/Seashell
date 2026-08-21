BIN := seashell
BLD_DIR := build/seashell
COMPCOM := compile_commands.json

SRC_DIR := src
DEP_DIR := deps

CC := clang

CFLAGS := -g -O0 -std=gnu23 -Wall -Wextra -fcolor-diagnostics
CPPFLAGS := -Iinc -I$(DEP_DIR)/linc_tools/inc
DEPFLAGS := -MMD -MP

SRCS := $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(DEP_DIR)/linc_tools/src/*.c)

OBJS := $(patsubst %.c,$(BLD_DIR)/%.o,$(SRCS))
DEPS := $(patsubst %.c,$(BLD_DIR)/%.d,$(SRCS))

COMP_FLAGS := $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS)

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(COMP_FLAGS) $^ -o $@

$(BLD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(COMP_FLAGS) -c $< -o $@

-include $(DEPS)
