CC = gcc
LD = gcc
CFLAGS = -O3 -g -fopenmp -Wall -Werror -march=native -funroll-loops -ffast-math -fopt-info-vec-optimized 
LDFLAGS = -lm -fopenmp
RM = /bin/rm -f
OBJS = galsim.o
EXECUTABLE = suduko_solver

all:$(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(LD) $(OBJS) -o $(EXECUTABLE) $(LDFLAGS)

galsim.o: galsim.c 
	$(CC) $(CFLAGS) -c galsim.c 

clean:
	$(RM) $(EXECUTABLE) $(OBJS) 

