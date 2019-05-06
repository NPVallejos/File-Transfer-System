all: server client

server:	server.o
	gcc nicholasvallejos_server.o -o server

client:	client.o
	gcc nicholasvallejos_client.o -o client

server.o:	nicholasvallejos_server.c packet.h
	gcc -c nicholasvallejos_server.c

client.o:	nicholasvallejos_client.c packet.h
	gcc -c nicholasvallejos_client.c
	
clean:
	rm -f *.o server client
