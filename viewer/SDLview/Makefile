#CC = clang
CC = gcc
FLAGS = -Wall -g3 -O3 -march=native -mtune=native 
CLIBS = `sdl-config --cflags` -I ~/include -I ../../net
LDFLAGS = `sdl-config --libs` -L ~/lib -lSDL_gfx -lSDL_image -lSDL_ttf -lm -lpthread -ljansson
OBJ = buffer.o json.o main.o sdl.o ../../net/pstr.o layerrenderers.o snapshot.o

ifeq ($(CC),gcc)
	FLAGS += -fms-extensions -std=c99
endif
ifeq ($(CC),clang)
	FLAGS += -D__STDC__ -Wno-empty-body -Wno-unknown-pragmas
endif

main: $(OBJ) Makefile
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CC) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean
clean:
	rm -f *.o main
