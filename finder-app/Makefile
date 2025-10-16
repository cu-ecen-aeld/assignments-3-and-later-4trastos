# Makefile para finder-app - compatible con Buildroot y cross-compilation

# Si CROSS_COMPILE está definido (por ejemplo aarch64-linux-gnu-), se usa para CC
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

CFLAGS ?= -Wall -Wextra -Werror -O2
LDFLAGS ?=
LDLIBS  ?=

TARGET := writer
SRCS   := writer.c
OBJS   := $(SRCS:.c=.o)

# Default target
.DEFAULT_GOAL := $(TARGET)

# Compilar y enlazar
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compilar .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets
.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
