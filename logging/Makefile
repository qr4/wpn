CC = gcc
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe -std=gnu99

all:
		$(CC) $(CFLAGS) -c logging.c -o logging.o
		# ar cr liblogging.a logging.o

.PHONY: clean

clean:
		-rm -f logging.o
		# liblogging.a
