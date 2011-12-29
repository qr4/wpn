CC = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -std=c99
CLIBS = `sdl-config --cflags` -I ~/include -I ../../net
LDFLAGS = `sdl-config --libs` -L ~/lib -lSDL_gfx -lSDL_image -lSDL_ttf -lm -lpthread -ljansson
OBJ = buffer.o json.o main.o sdl.o ../../net/pstr.o

main: $(OBJ)
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CC) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean
clean:
	rm -f *.o main
