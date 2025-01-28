all: writer

writer: writer.c
	${CROSS_COMPILE}gcc -g -Wall -o writer writer.c

clean:
	rm -f writer *.o