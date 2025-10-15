# Cross-compile settings
CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc

CFLAGS  ?= -Wall -Wextra -Werror -O2 -g
LDFLAGS ?=
LDLIBS  ?=

TARGET := writer
SRCS   := writer.c
OBJS   := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f $(TARGET) $(OBJS)
