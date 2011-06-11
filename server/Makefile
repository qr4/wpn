
OBJ = main.o entities.o vector.o map.o ship.o luastate.o luafuncs.o physics.o config.o entity_storage.o storages.o json_output.o route.o

all: $(OBJ)

include ../settings.mak

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY : clean debug
clean:
	rm -f *.s *.o main

