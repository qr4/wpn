CC = gcc
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config5.1 lua --cflags` -std=gnu99
LDFLAGS = -lm -lrt `pkg-config5.1 lua --libs` -Wl,-O1 -Wl,--as-needed
OBJ = main.o entities.o vector.o map.o ship.o luastate.o luafuncs.o physics.o config.o entity_storage.o storages.o json_output.o route.o

all: $(OBJ)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY : clean debug
clean:
	rm -f *.s *.o main

debug: FLAGS += -DENABLE_DEBUG

debug: main
