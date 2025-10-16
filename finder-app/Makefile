# Makefile para finder-app - Buildroot compatible
CC ?= gcc
CFLAGS ?= -Wall -Werror

all: writer

writer: writer.c
	$(CC) $(CFLAGS) -o writer writer.c

clean:
	rm -f writer

.PHONY: all clean