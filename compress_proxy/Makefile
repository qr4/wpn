CC = clang
FLAGS = -Wall -g3 -O2 -march=native -mtune=native
CLIBS = 
LDFLAGS = -lev
CFILES = $(wildcard *.c)
OBJ = $(CFILES:.c=.o)

main: $(OBJ)
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ) 

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $< 

%.o: %.c 
	$(CC) $(FLAGS) $(CLIBS) -c $< 

.PHONY : clean
clean:
	rm -f *.o main
