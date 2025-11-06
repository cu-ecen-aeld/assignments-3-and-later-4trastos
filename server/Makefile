CC ?= gcc
CFLAGS ?= -Wall -Wextra -pthread -lrt
TARGET = aesdsocket
SRC = aesdsocket.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
