CC = gcc
CFLAGS = -O2 -Wall -I./include -lm

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: RCT client server
RCT: RCT.c csapp.o
	$(CC) $(CFLAGS)  -o RCT RCT.c csapp.o $(LIB)

client: example_client.c csapp.o
	$(CC) $(CFLAGS) -o client example_client.c csapp.o $(LIB)

server: fs_server.c csapp.o
	$(CC) $(CFLAGS) -o server fs_server.c csapp.o $(LIB)

csapp.o: lib/csapp.c
	$(CC) $(CFLAGS) -c lib/csapp.c

clean:
	rm -f *.o *~
