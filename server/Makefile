CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
TARGET = aesdsocket
SRC = aesdsocket.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
