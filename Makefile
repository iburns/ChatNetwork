all: server client

client: client.o

client.o: client.c
	gcc -g -c client.c

server: server.o
	gcc -g -o server server.o

server.o: server.c
	gcc -g -c server.c

