CC ?= gcc
CFLAGS = -g -Wall
TARGET = aesdsocket
HEADER_FILES = aesdsocket.h singly_linked_list.h
SOURCE_FILES = singly_linked_list.c

default: all
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(HEADER_FILES) -o $(TARGET) $(TARGET).c $(SOURCE_FILES)

clean:
	rm -rf $(TARGET) *.o
