CC= gcc
CFLAGS=-Wpedantic -Werror -pthread

all: client server

client: client.o image.o
	$(CC) $(CFLAGS) -g -o client client.o image.o

server: server.o handlesockets.o
	$(CC) $(CFLAGS) -g -o server server.o handlesockets.o

%.o: %.c
	$(CC) $(CFLAGS) -g -c $^ -o $@ 

clean:
	-rm -rf *.o client server
