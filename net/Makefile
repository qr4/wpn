CC = gcc
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe -std=gnu99 -I../logging
LDFLAGS = -lrt -lssl -lcrypto
OBJ_NET = network.o pstr.o net.o net_test.o ../logging/logging.o
OBJ_TALK = network.o pstr.o talk_test.o userstuff.o dispatch.o talk.o ../logging/logging.o

all: talk net

dbg: all

talk: $(OBJ_TALK)
		$(CC) -o $@ $(OBJ_TALK)  $(LDFLAGS)

net: $(OBJ_NET)
		$(CC) -o $@ $(OBJ_NET) $(LDFLAGS)

%.o: %.c %.h
		$(CC) $(CFLAGS) -c $<

%.o: %.c
		$(CC) $(CFLAGS) -c $<

.PHONY : clean
clean:
		rm -f *.o net talk
