CFLAGS=-Wall -g -O2
TARGET := tiny6502
ifeq ($(OS), Windows_NT)
	CC=gcc-mingw-w64
	TARGET += .exe
else
	ifeq ($(shell uname), Darwin)
		CC=clang
	else
		CC=gcc
	endif
endif

SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

.PHONY: clean

cleanall: 
ifeq ($(OS), Windows_NT)
	del $(BUILD_DIR)\$(TARGET) 
	del $(OBJ_DIR)\*.o
else
	rm -r $(BUILD_DIR)/$(TARGET)
	rm -r $(OBJ_DIR)/*.obj
endif

clean:
ifeq ($(OS), Windows_NT)
	del $(OBJ_DIR)\*.o
else
	rm -r $(OBJ_DIR)/*.o
endif