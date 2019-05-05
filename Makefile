all: server client

server:	server.o
	gcc server.o -o server

client:	client.o
	gcc client.o -o client

server.o:	server.c packet.h
	gcc -Wall -Wextra -Wpedantic -c server.c

client.o:	client.c packet.h
	gcc -Wall -Wextra -Wpedantic -c client.c
	
clean:
	rm -f *.o server client
