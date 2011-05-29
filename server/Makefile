CC = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config lua5.1 --cflags`
CLIBS =
LDFLAGS = -lm `pkg-config lua5.1 --libs` -Wl,-O1 -Wl,--as-needed
OBJ = main.o entities.o vector.o map.o ship.o luastate.o luafuncs.o physics.o route.o

main: $(OBJ)
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ) 

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $< 

%.o: %.c 
	$(CC) $(FLAGS) $(CLIBS) -c $< 

.PHONY : clean
clean:
	rm -f *.s *.o main
