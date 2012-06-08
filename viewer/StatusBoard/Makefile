#CC = clang
CC = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -D_GNU_SOURCE=1 -D_REENTRANT
CLIBS = -I ~/include -I ../../net
LDFLAGS = -L ~/lib -lm -lpthread -ljansson
LOCALOBJ = statusboard.o
REMOTEOBJ = ../ClientLib/buffer.o ../ClientLib/json.o ../ClientLib/main.o ../ClientLib/snapshot.o ../../net/pstr.o

ifeq ($(CC),gcc)
	FLAGS += -fms-extensions -std=c99
endif
ifeq ($(CC),clang)
	FLAGS += -D__STDC__ -Wno-empty-body
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
