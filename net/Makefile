CC = gcc
CFLAGS = -std=gnu99 -DNET_HAVE_MAIN -I../logging -g3
LDFLAGS = -lrt
OBJ = network.o net.o ../logging/logging.o


net: $(OBJ)
		$(CC) $(LDFLAGS) -o $@ $(OBJ) 

%.o: %.c %.h
		$(CC) $(CFLAGS) -c $< 

%.o: %.c 
		$(CC) $(CFLAGS) -c $< 

.PHONY : clean
clean:
		rm -f *.o net


