CXX = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native
CLIBS = `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lSDL_gfx  -lm -lpthread
OBJ = main.o screen.o route.o json_output.o

main: $(OBJ)
	$(CXX) $(FLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c %.h
	$(CXX) $(FLAGS) $(CLIBS) -c $< 

%.o: %.c 
	$(CXX) $(FLAGS) $(CLIBS) -c $< 

.PHONY : clean
clean:
	rm -f *.o main
