#include <sys/socket.h> // needed for socket programming
#include <sys/types.h> 	// needed for socket programming
#include <sys/stat.h>	// for stat()
#include <netinet/in.h>	// for printing an internet address - useful for debugging
#include <arpa/inet.h> 	// for inet_pton
#include <netdb.h> 		// For gethostbyaddr() function
#include <stdlib.h>
#include <stdio.h>
#include <string.h>		// for string related functions like bzero
#include <unistd.h>		// for close(), read(), and write()
#include <fcntl.h>
#include <sys/time.h>	// for struct timeval
#include "packet.h"		// includes definition for Packet struct and contains MAX macro

#define PACKET 1 	// This will print the packet size (line 35)
#define TIMEOUT 5	// Specifies how many seconds to wait for timeout mechanism

int 
main(int argc, char ** argv) {
	if(argc < 4) {
		printf("Please enter filename, hostname, and port #.\n");
		return -1;
	}

#if PACKET
	printf ("-------------------------\n Packet size = %lu bytes\n-------------------------\n", sizeof(Packet));
#endif

	int sockfd; // this will hold our socket file description	
	unsigned int bytes;	// total number of bytes sent

	struct sockaddr_in servaddr; // holds server information
	socklen_t alen; 	// hold servaddr size
	int port = atoi(argv[3]); 	// port between client and server will be the same in program
	char * hostname = argv[2]; // used to find the servers ip address
	struct hostent *hp; // host information of server	
	
	FILE * fptr = NULL;	// Used for opening the file
	const char * filename = argv[1];	// hold filename
	int filesize; // Used for determining the number of packets needed for file transfer
	int filetype;	// Used for switch case later when parsing file
	//char buffer[MAX]; // Used for reading file
	unsigned int totalpackets;
	int remainder = 0; // filesize % MAX-1 bytes leftover (if any)

	/* Setting up timer for timeout mechanism */
	struct timeval tv; 
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	
	/* Opening the file */
	fptr = fopen(filename, "rb");
	
	/* Check if file was opened successfully*/
	if (fptr == NULL) {
		perror ("Could not open the file");
		fclose(fptr);
		return 0;
	}		

	/* Figure out the size of the file in bytes*/
	struct stat st;
	stat(filename, &st);
	filesize = st.st_size;
	printf(" Filesize = %d bytes\n", filesize);
	printf("-------------------------\n\n");

	/* Figure out how many packets are needed based on filesize */
	totalpackets = filesize / (MAX-1);
	if ((remainder = filesize % (MAX-1)) > 0) { ++totalpackets; } // if filesize is not evenly divisible by MAX size then we increment totalpackets by 1
	++totalpackets; // increment one more time for a packet that contains filename and filesize
	
	// Pinting out some information
	printf("totalpackets=%lu\n", totalpackets);
	printf("remainder=%d\n", remainder);	
	printf("MAX=%lu\n", MAX);

	/* Step 1: Create the socket */
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("cannot create socket");
		close(sockfd);
		return 0;
	}
	
	// Make the recvfrom sockfd able to timeout using SO_RCVTIMEO and struct timeval "tv"
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0) {
		perror ("setsockopt failed");
		close(sockfd);
		return 0;
	}

	/* Step 2. Get server information */
	hp = gethostbyname(hostname); // lookup address by hostname
	if (!hp) {
		printf ("gethostbyname: failed to get address given hostname");
		close(sockfd);
		return 0;
	}
	
	/* Setup the sockaddr_in struct */
	memset ((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 	// This means ipv4 address
	servaddr.sin_port = htons(port);	// converting port to a short/network byte order
	memcpy ((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length); 	// now we can put the host's address into the servaddr struct
	
	printf("Server Host Name: %s\n", hp->h_name);
	
	alen = sizeof (servaddr);

	/* Step 3: Send packets to server */
	unsigned int count = 0;
	while (count < totalpackets) {
		Packet packet;  // Packet to send out
		Packet ack;		// Packet received from server
				
		if (count == 0) {
			/* Creating packet that holds filename, totalpackets, and "remainder" bytes (if any) */
			packet.hp.opcode = 2; // Make packet  RRQ
			packet.hp.sequenceNumber = count; // Order by index
			memset(packet.data, 0, MAX); // zero out char array


			/* This is what packet.data will look like after this block of code => "<filename>\t<totalpackets>\t<remainder>"*/
			strcat(packet.data, filename); 
			strcat(packet.data, "\t");
					
			char * str = malloc (sizeof (char *)); // Used to hold converted integer data
	
			sprintf(str, "%d", totalpackets); 	// convert totalpackets into a string
			strcat(packet.data, "\t");	
			strcat(packet.data, str);
				
			sprintf(str, "%d", remainder);		// convert remainder into a string
			strcat(packet.data, "\t");
			strcat(packet.data, str);
				
			free(str);
		}
		else {
			/* Here we are transferring file data into packet */
			packet.hp.opcode = 1; // Make packets WRQ
			packet.hp.sequenceNumber = count; // Order by index
			memset(packet.data, 0, MAX); // zero out char array	
			
			// If we are at our last packet AND filesize is not evenly divisible by MAX-1 we read remainder bytes
			if (count == totalpackets - 1 && remainder > 0) {
				fread(packet.data, 1, remainder, fptr);
			}
			// Otherwise we read in MAX-1 bytes (we subtract 1 to leave room for the null terminator in our data array)
			else {
				fread(packet.data, 1, MAX-1, fptr);	
			}
		}

		// Now we send the packet and wait for an ACK (loop until ACK found)
		while (1) {
			/* Sending packet to server */
			if (sendto (sockfd, &packet, sizeof packet, 0, (struct sockaddr *)&servaddr, alen) == -1) {
				perror("sendto, client-side");
				close(sockfd);
				return 0;
			}

			/* Waiting for ACK packet from server */
			if ((bytes = recvfrom (sockfd, &ack, sizeof ack, 0, (struct sockaddr *)&servaddr, &alen)) == -1) {
				printf("Did not receive ACK...trying again\n");
				continue;
			}
				
			/* Check if ACK packet contains same sequenceNumber */
			if (ack.hp.opcode == 0 && ack.hp.sequenceNumber == packet.hp.sequenceNumber) {
				break;
			}
		}
		++count;
	}	

	/* Step 5: Tell server to stop waiting for packets */
	Packet dummy;
	dummy.hp.opcode = 0;
	if ((bytes = sendto(sockfd, &dummy, sizeof dummy, 0, ((struct sockaddr *)&servaddr), alen)) == -1) {
		perror("sendto");
		return 0;
	}
	
	close(sockfd); // Close the socket
	fclose(fptr);	// close the file

	return 0;
}
