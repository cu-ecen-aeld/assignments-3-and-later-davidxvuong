CC ?= gcc
CFLAGS = -g -Wall
TARGET = writer

default: all
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -g -Wall -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET) *.o