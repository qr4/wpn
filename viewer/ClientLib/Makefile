#CC = clang
CC = gcc
FLAGS = -Wall -g3 -O3 -march=native -mtune=native
CLIBS = -I ~/include -I ../../net
LDFLAGS = -L ~/lib -lm -lpthread -ljansson
OBJ = buffer.o json.o main.o ../../net/pstr.o snapshot.o

ifeq ($(CC),gcc)
	FLAGS += -fms-extensions -std=c99
endif
ifeq ($(CC),clang)
	FLAGS += -D__STDC__ -Wno-empty-body -Wno-unknown-pragmas
endif

all: $(OBJ) Makefile
#	$(CC) $(FLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $<

%.o: %.c
	$(CC) $(FLAGS) $(CLIBS) -c $<

.PHONY : clean
clean:
	rm -f *.o
