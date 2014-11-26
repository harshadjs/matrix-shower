CFLAGS=-g
CC=gcc
all:
	$(CC) $(CFLAGS) matrix.c -o matrix
clean:
	rm -f matrix
