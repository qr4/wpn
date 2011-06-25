CXX = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -std=gnu99
CLIBS = `sdl-config --cflags` -I ~/include
LDFLAGS = `sdl-config --libs` -L ~/lib -lSDL_gfx -lSDL_image -lSDL_ttf -lm -lpthread -ljansson
OBJ = buffer.o json.o main.o sdl.o

main: $(OBJ)
	$(CXX) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CXX) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CXX) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean
clean:
	rm -f *.o main
