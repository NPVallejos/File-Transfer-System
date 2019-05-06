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

int 
main(int argc, char ** argv) {
	if (argc < 2) {
		perror("missing port number in cmd args");
		return 0;
	}

	int sockfd;	// listening socket
	int port = atoi(argv[1]);	// WARNING: atoi() is not safe; 	// must be specified in cmd args
	char hostname[128]; // Stores the hostname
	struct hostent *hp; // host information of server
	struct sockaddr_in myaddr; // holds information about this server 
	struct sockaddr_in	client; // holds client addr
	socklen_t alen; 		// length of a single client address struct
	Packet packet;// = malloc (sizeof (Packet *));
	
	char * filename = malloc (sizeof (char *));
	int bytes;	// holds the number of bytes received
	int remainder = 0;		// Hold how many bytes of data are being sent in the last packet when filesize % MAX-1 != 0
	int totalpackets = 0;	// Hold how many unique packets are being sent
	int last_seqnum = -1;	// Keep track of the last frame number 
	int c = 0;				// Keep track of how many packets we have received so far
	FILE * fptr = NULL;		// File pointer
	
	// Step 1: Create a socket
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) { // May have issues with setting protocol to 0
		perror("cannot create socket");
		close(sockfd);
		return 0;
	}

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
	printf ("server started on [hostname] %s. Waiting for messages on port %d (ip=%s)...\n-----------------------------------\n", hostname, myaddr.sin_port, inet_ntoa(myaddr.sin_addr));

	alen = sizeof (client);
	
	/* Step 3: obtain data from client - recvfrom should be blocking AND should loop until server receives a packet with less than the size of a normal data block */
	while ((bytes = recvfrom (sockfd, &packet, sizeof (packet), 0, (struct sockaddr *)&client, &alen)) != -1) {
		//if (sizeof packet != 1500) { continue; }
		switch (packet.hp.opcode) {
			case 1:
				{
					printf("Packet number %u received. ", packet.hp.sequenceNumber);
					// WRQ - write data to file
					if (packet.hp.sequenceNumber != last_seqnum) {
						++c;
						last_seqnum = packet.hp.sequenceNumber;
							
						if (fptr != NULL) {
							if (c == totalpackets - 1 && remainder > 0) { fwrite(packet.data,  1, remainder, fptr); }
							else { fwrite(packet.data,  1, bytes-sizeof (Header) - 1, fptr); }
						}
						printf("Sending ACK\n");
					}
					else {
						printf("Already received this packet...Resending ACK\n");
					}
			
					// Sending packet with ACK
					Packet sendpack;
					sendpack.hp.opcode = 0;
					sendpack.hp.sequenceNumber = packet.hp.sequenceNumber;
					sendto (sockfd, &sendpack, sizeof (sendpack), 0, (struct sockaddr *)&client, alen);
				}
				break;
			case 2:
				{
					printf("Packet number %u received. ", packet.hp.sequenceNumber);
					// RRQ - read data -> contains filename and filesize
					if (packet.hp.sequenceNumber != last_seqnum) {
						++c;
						last_seqnum = packet.hp.sequenceNumber;

						filename = strtok(packet.data, "\t");
						totalpackets = atoi(strtok(NULL, "\t"));
						remainder = atoi(strtok(NULL, "\t"));
				
						// Opening file
						fptr = fopen(filename, "wb");
					
						if (fptr == NULL) {
							perror("failed to open file");
							return 0;
						}
						printf("Sending ACK\n");
					}
					else {
						printf("Already received this packet...Resending ACK\n");		
					}

					// Sending packet with ACK
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
    }

	free (filename);
	fclose(fptr); 	// close the file
	close(sockfd); 	// close the listening socket
	return 0;
}
