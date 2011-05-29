CC = gcc
CFLAGS = -std=gnu99 -DNET_HAVE_MAIN -I../logging -g3
LDFLAGS = -lrt
OBJ_NET = network.o net.o ../logging/logging.o
OBJ_TALK = network.o talk.o ../logging/logging.o

all: talk net

talk: $(OBJ_TALK)
		$(CC) $(LDFLAGS) -o $@ $(OBJ_TALK)

net: $(OBJ_NET)
		$(CC) $(LDFLAGS) -o $@ $(OBJ_NET) 

%.o: %.c %.h
		$(CC) $(CFLAGS) -c $< 

%.o: %.c 
		$(CC) $(CFLAGS) -c $< 

.PHONY : clean
clean:
		rm -f *.o net talk


