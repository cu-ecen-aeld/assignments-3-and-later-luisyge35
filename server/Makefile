CC := gcc
CROSS_COMPILE := aarch64-none-linux-gnu-
TARGET := aesdsocket

ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
	CC := $(CROSS_COMPILE)$(CC)
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): aesdsocket.o
	$(CC) -o $(TARGET) aesdsocket.o

aesdsocket.o: aesdsocket.c
	$(CC) -c aesdsocket.c

clean:
	rm -f $(TARGET) aesdsocket.o