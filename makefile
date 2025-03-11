CC = gcc
LD = gcc
CFLAGS = -O3 -g -fopenmp -Wall -Werror -march=native -funroll-loops -ffast-math -fopt-info-vec-optimized 
LDFLAGS = -lm -fopenmp
RM = /bin/rm -f
OBJS = main.o
EXECUTABLE = suduko_solver

all:$(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(LD) $(OBJS) -o $(EXECUTABLE) $(LDFLAGS)

main.o: main.c 
	$(CC) $(CFLAGS) -c main.c 

clean:
	$(RM) $(EXECUTABLE) $(OBJS) 

