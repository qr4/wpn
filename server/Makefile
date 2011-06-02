CC = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config lua5.1 --cflags` -std=gnu99
CLIBS =
LDFLAGS = -lm -lrt `pkg-config lua5.1 --libs` -Wl,-O1 -Wl,--as-needed
OBJ = main.o entities.o vector.o map.o ship.o luastate.o luafuncs.o physics.o config.o entity_storage.o storages.o json_output.o route.o

all: main

main: $(OBJ)
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CC) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean debug
clean:
	rm -f *.s *.o main

debug: FLAGS += -DENABLE_DEBUG

debug: main
