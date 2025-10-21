CC := gcc
CFLAGS := -Wall -Werror -Wextra -g -pthread
LDFLAGS := -pthread
TARGET := aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
	sudo rm -f /var/tmp/aesdsocketdata

.PHONY: all clean
default: all