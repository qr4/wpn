#CC = clang
CC = gcc
FLAGS = -Wall -g3 -O3 -march=native -mtune=native 
CLIBS = `sdl-config --cflags` -I ~/include -I ../../net
LDFLAGS = `sdl-config --libs` -L ~/lib -lSDL_gfx -lSDL_image -lSDL_ttf -lm -lpthread -ljansson
LOCALOBJ = sdl.o layerrenderers.o
REMOTEOBJ = ../ClientLib/buffer.o ../ClientLib/json.o ../ClientLib/main.o ../ClientLib/snapshot.o ../../net/pstr.o

ifeq ($(CC),gcc)
	FLAGS += -fms-extensions -std=c99
endif
ifeq ($(CC),clang)
	FLAGS += -D__STDC__ -Wno-empty-body -Wno-unknown-pragmas
endif

main: $(LOCALOBJ) $(REMOTEOBJ) Makefile ClientLib
	$(CC) $(FLAGS) -o $@ $(LOCALOBJ) $(REMOTEOBJ) $(LDFLAGS)

ClientLib:
	$(MAKE) -C ../ClientLib

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CC) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean
clean:
	rm -f *.o main
