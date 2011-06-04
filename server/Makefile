CC = gcc
LUA_PKG = $(shell pkg-config --list-all | grep lua | head -n 1 | cut -d" " -f 1)
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config $(LUA_PKG) --cflags` -std=gnu99
LDFLAGS = -lm -lrt `pkg-config $(LUA_PKG) --libs` -Wl,-O1 -Wl,--as-needed
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
