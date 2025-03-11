CC ?= gcc
CFLAGS ?= -g -Wall -I../aesd-char-driver

# Conditionally add -DUSE_AESD_CHAR_DEVICE to CFLAGS
ifeq ($(USE_AESD_CHAR_DEVICE), 1)
CFLAGS += -DUSE_AESD_CHAR_DEVICE
endif

TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt
HEADER_FILES = aesdsocket.h singly_linked_list.h ../aesd-char-driver/aesd_ioctl.h
SOURCE_FILES = singly_linked_list.c

default: all
all: $(TARGET)

$(TARGET): $(TARGET).c $(HEADER_FILES) $(SOURCE_FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(SOURCE_FILES) $(LDFLAGS)

clean:
	rm -rf $(TARGET) *.o
