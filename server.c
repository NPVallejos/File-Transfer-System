/* Server needs to retrieve its information,
	Neeed to create a socket()
	Need to bind() sockfd 
	Use recvfrom() to get data and store the numbytes that this returns to guarantee is 1500 B - also should define a MAXBUFFLENGTH */
	
#include <sys/socket.h>	// needed for socket functions & recvfrom
#include <sys/types.h>	// neeeded for socket functions & recvfrom
#include <netinet/in.h> // for printing an internet address
#include <arpa/inet.h> 	// for printing an internet address
#include <netdb.h>		// for gethostbyname
#include <stdlib.h>
#include <stdio.h>	
#include <errno.h>	// can be used in conjunction with perror
#include <string.h> 	// for str related functions (i.e. bzero)
#include <stdbool.h> // for bool types
#include <unistd.h> // for close(), gethostname(), read(), write()
#include <sys/time.h>
#include "packet.h" // contains definition for Packet struct and MAX macro

#define TIMEOUT 100000

void * get(struct sockaddr *s) {
	return &(((struct sockaddr_in *)s)->sin_addr);
}

int 
main(int argc, char ** argv) {
	if (argc < 2) {
		perror("missing port number in cmd args");
		return 0;
	}

	int sockfd;	// listening socket
	int port = atoi(argv[1]);	// WARNING: atoi() is not safe; 	// must be specified in cmd args
	int bytes;	// holds the number of bytes received
	char hostname[128]; // Stores the hostname
	struct hostent *hp; // host information of server
	struct sockaddr_in myaddr; // holds information about this server 
	struct sockaddr_in	client; // holds client addr
	socklen_t alen; 		// length of a single client address struct
	int sockoptval = 1; 	// used in conjunction with setsockopt
	Packet packet;// = malloc (sizeof (Packet *));
	char message[MAX];
	
	// Step 1: Create a socket
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) { // May have issues with setting protocol to 0
		perror("cannot create socket");
		close(sockfd);
		return 0;
	}
	
	// allow immediate reuse of PORT
//	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(int));

	/* Step 2: Identify (name) a socket */
	memset ((char *) &myaddr, 0, sizeof(myaddr));
	
	gethostname (hostname, sizeof(hostname));	// get hostname of server
	hp = gethostbyname (hostname); // return a struct hostent given server hostname
	if (!hp) {
		perror ("gethostbyname: failed to get host name");
		close(sockfd);
		return 0;
	}
	
	// Fill in fields for struct sockaddr_in myaddr
	myaddr.sin_family = AF_INET; 	// This means ipv4 address
	myaddr.sin_port = htons (port);	// converting PORT to a short/network byte order
	memcpy ((void *)&myaddr.sin_addr, hp->h_addr_list[0], hp->h_length); // now we can put the host's address into the servaddr struct
	
	// now bind socket sockfd to myaddr
	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror ("bind failed");
		close(sockfd);
		return 0;
	}
	
	// Print server information
	printf ("server started on [hostname] %s, listening on port %d (ip=%s)\n-----------------------------------\n", hostname, myaddr.sin_port, inet_ntoa(myaddr.sin_addr));

	alen = sizeof (client);
	
	
	/* Step 3: obtain data from client - recvfrom should be blocking AND should loop until server receives a packet with less than the size of a normal data block */
	char * filename = malloc (sizeof (char *));
	int filesize = 0;
	int last_seqnum = -1;
	int c = 0;
	int filetype = 0;
	FILE * fptr = NULL;
	while ((bytes = recvfrom (sockfd, &packet, sizeof (packet), 0, (struct sockaddr *)&client, &alen)) != -1) {	
		//if (c >= 14) {	}//break; }
		if (sizeof packet != 1500) { continue; }
		//char dummy[128];
		//printf("%d, %d\n", packet.hp.opcode, packet.hp.sequenceNumber);
		switch (packet.hp.opcode) {
			case 1:
				{
				//printf("Received packet %d\n", packet.hp.sequenceNumber);
				// WRQ - write data to file
				if (packet.hp.sequenceNumber != last_seqnum) {
					++c;
					last_seqnum = packet.hp.sequenceNumber;
					
					if (fptr != NULL) {
						if (filetype == 0) { fwrite(packet.data, sizeof(char), strlen(packet.data), fptr); }
						else { fwrite(packet.data, 1, strlen(packet.data), fptr); }
					}

					/* Client information */
					//printf("Client Information: %s\n", inet_ntop(client.ss_family, 
					//										get((struct sockaddr *)&client), 
					//										dummy, sizeof dummy)); // converting client info from network to printable
					/* Message information */
					//printf("=========\n%s", packet.data);
					// ERROR: strlen(packet.data returns 1497 and packet.data does not return the last character)
					//printf("Packet Length = %d bytes\nPacket data length = %d\nPacket content: \n%s\n", bytes, strlen(packet.data), packet.data);
				}
				else {
					printf("resending ACK\n");
				}
				Packet sendpack;
				sendpack.hp.opcode = 0;
				sendpack.hp.sequenceNumber = packet.hp.sequenceNumber;
				sendto (sockfd, &sendpack, sizeof (sendpack), 0, (struct sockaddr *)&client, alen);
				}
				break;
			case 2:
				{
				// RRQ - read data -> contains filename and filesize
				filename = strtok(packet.data, "\t");
				filesize = atoi(strtok(NULL, "\t"));
			
				/* Here we figure out what type of file we are writing to */
				const char * result;
				if ((result = strchr(filename, '.')) != NULL) {
					++result;
					if (strcmp(result, "txt") == 0) {
						fptr = fopen("test.txt", "w");
						filetype = 0;
					}
					else if (strcmp(result, "mp4") == 0) {
						//printf("Opening %s for writing in binary\n", filename);
						fptr = fopen("test.mp4", "wb");
						filetype = 1;
					}
				}

				Packet sendpack;
				sendpack.hp.opcode = 0;
				sendpack.hp.sequenceNumber = packet.hp.sequenceNumber;
				sendto (sockfd, &sendpack, sizeof (sendpack), 0, (struct sockaddr *)&client, alen);
				}
				break;
			default:
				printf("Done\n");
				return 0;
		}
		//bzero(dummy, sizeof dummy);
    }

	fclose(fptr);
	close(sockfd); // close the listening socket
	return 0;
}
