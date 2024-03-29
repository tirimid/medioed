.PHONY: all

SRC_DIR := src
INC_DIR := include
LIB_DIR := lib

CC := gcc
LD := gcc
CFLAGS := -std=c99 -pedantic -I$(INC_DIR) -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE
LDFLAGS :=

SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(patsubst $(SRC_DIR)/%,$(LIB_DIR)/%.o,$(SRCS))
OUT_BIN := medioed

all: $(OUT_BIN)

$(OUT_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(LIB_DIR)/%.o: $(SRC_DIR)/%
	@ mkdir -p $@
	@ rmdir $@
	$(CC) $(CFLAGS) -o $@ -c $<
