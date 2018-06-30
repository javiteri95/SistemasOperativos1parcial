CC = gcc
CFLAGS = -O2 -Wall -I./include -lm

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: RCT
RCT: RCT.c csapp.o
	$(CC) $(CFLAGS)  -o RCT RCT.c csapp.o $(LIB)
	rm -f *.o

csapp.o: lib/csapp.c
	$(CC) $(CFLAGS) -c lib/csapp.c

clean:
	rm -f *.o RCT *~
