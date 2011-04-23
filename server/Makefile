CC = gcc
FLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe
CLIBS =
LDFLAGS = -lm 
OBJ = main.o entities.o vector.o map.o

main: $(OBJ)
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $(OBJ) 

%.o: %.c %.h
	$(CC) $(FLAGS) $(CLIBS) -c $< 

%.o: %.c 
	$(CC) $(FLAGS) $(CLIBS) -c $< 

.PHONY : clean
clean:
	rm -f *.s *.o main
