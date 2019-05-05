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

#define NDEBUG 0 // Enables debug printf's that I wrote
#define PACKET 1 // This will print the packet size (line 35)
#define TIMEOUT 5

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
	char buffer[MAX]; // Used for reading file
	unsigned int totalpackets;
	int remainder;

	struct timeval tv; // Used for timeout mechanism
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	/* Here we figure out what type of file we are reading from */
	const char * result; // temp variable used for strchr()
	if ((result = strchr(filename, '.')) != NULL) {
		printf("-------------------------\n ");
		++result;
		if (strcmp(result, "txt") == 0) {
			printf("Text file received\n ");
			fptr = fopen(filename, "r");
			filetype = 0;
		}
		else if (strcmp (result, "mp4") == 0) {
			printf("MP4 file received\n ");
			fptr = fopen(filename, "rb");
			filetype = 1;
		}
	}
	
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

	/* Divide file into packets */
	totalpackets = filesize / (MAX-1);
	if ((remainder = filesize % (MAX-1)) > 0) { ++totalpackets; } // if filesize is not evenly divisible by MAX size then we increment totalpackets by 1
	
	
	++totalpackets; // increment one more time for a packet that contains filename and filesize
	
	/* 
	   Now we can get our Packets ready for sending 
	   Note: packets[0] is reserved for holding filename & filesize
	*/	
	printf("%lu\n", totalpackets);
	Packet packets[totalpackets];
	printf("remainder=%d\n", remainder);
	unsigned int count;
	
	/* Initialize all fields */
	for (count = 0; count < totalpackets; ++count) {
		if (count != 0) {
			packets[count].hp.opcode = 1; // Make packets WRQ
			packets[count].hp.sequenceNumber = count; // Order by index
			memset(packets[count].data, 0, MAX); // zero out char array
			//printf("%s\n",packets[count].data);
		}
		else {
			packets[count].hp.opcode = 2; // Make packet[0]  RRQ
			packets[count].hp.sequenceNumber = count; // Order by index
			memset(packets[count].data, 0, MAX); // zero out char array

			char * str = malloc (sizeof (char *));
			sprintf(str, "%d", filesize);

			/* packet[0].data => "<filename>\t<filesize>"*/
			strcat(packets[count].data, filename); 
			strcat(packets[count].data, "\t");
			strcat(packets[count].data, str);
			
			free(str);
		}
	}



	printf("MAX=%lu\n", MAX);
	printf("sizeof packets[%d].data = %lu\n", totalpackets-1, strlen(packets[totalpackets-1].data));
	printf("sizeof packets[%d].data = %lu\n", totalpackets-2, strlen(packets[totalpackets-2].data));
//	for (count = 0; count < totalpackets; ++count) {
//		printf("count=%d, %d, ", count, packets[count].hp.opcode); // Make packets WRQ
//		printf("%d\n", packets[count].hp.sequenceNumber); // Order by index
//	}

	count = 1;
	while (count < totalpackets) {
		#if NDEBUG
			printf("iteration %d\n", count);
		#endif
		bytes = 0;
		switch (filetype) {
			case 0:
			{
				//char temp[100];
				// Reading a text file
				//Packet pack;
				while (strlen(packets[count].data) < MAX-1) {
					char * s = fgets(buffer, 2, fptr); 
					//bytes += strlen(buffer);
					//printf("BEFORE:\nsizeof data==%d\n", strlen(packets[count].data));
					if (MAX-strlen(packets[count].data) == 1) {
						printf("buffer=%s\n", s);
						break;
					}
					strcat(packets[count].data, buffer); // append lines to char data array
					//printf("AFTER:\nsizeof data==%lu\n", strlen(packets[count].data));
					if (MAX-strlen(packets[count].data) == 1) {
					//	printf("buffer=%s\n", buffer);
						//return 0;
					}
					//packets[count].hp.opcode = 1;
					bzero(buffer, MAX);
					if (count == totalpackets-1 && strlen(packets[count].data) > remainder-1) {
						break;
					}
				}

				//printf("%lu\n", sizeof packets[count].data);
				/* Sending data to server using packets */
				if ((bytes = sendto (sockfd, &packets[count], sizeof packets[count], 0, (struct sockaddr *)&servaddr, alen)) == -1) {
					perror("sendto, client-side");
					close(sockfd);
					return 0;
				}
#if 0
				//printf("Sent to client\n");
				/* TODO: add a time-out mechanism using pthreads */
				/* Waiting for ACK from server */
				if ((bytes = recvfrom (sockfd, &ack, sizeof ack, 0, (struct sockaddr *)&servaddr, &alen)) == -1) {
					printf("Did not receive ACK...trying again\n");
					continue;
					//perror("Did not receive ACK");
					//return 0;
				}
				//	printf("ack.hp.opcode==%d = 0 && ack.hp.sequenceNumber == %d == %d\n", ack.hp.opcode, ack.hp.sequenceNumber, packets[count].hp.sequenceNumber);
				/* Check if ACK packet contains same sequenceNumber */
				if (ack.hp.opcode == 0 && ack.hp.sequenceNumber == packets[count].hp.sequenceNumber) {
					++count; // Successfully sent out packet
				}
#endif
			}
			  break;
			case 1:
				// Reading a binary file
				while (strlen(packets[count].data) < MAX-1) {
					char buff[MAX];
					fread (buff, 1, 1, fptr);
					unsigned int len = strlen(packets[count].data);
					//strcat(packets[count].data, buff);
					
					//printf("%lu\n", strlen(buff));
					memcpy(packets[count].data + len, buff, strlen(buff));
					
					//return;
					if (count == totalpackets - 1 && strlen(packets[count].data) > remainder - 1) {
						break;
					}
				}
				break;
			default:
				return 0;
		}
		++count; // only move on to next packet when 'data' array has been filled up completely
	}
//	for (count = 0; count < totalpackets; ++count) {
//		printf("\ncount=%d, opcode=%d, seqnum=", count, packets[count].hp.opcode); // Make packets WRQ
//		printf("%d, data size=%lu\n", packets[count].hp.sequenceNumber, strlen(packets[count].data)); // Order by index
//		//printf("%s", packets[count].data);
//	}

#if NDEBUG
	count = 0;
	while (count < totalpackets) {
		printf("sizeof packets[%d].data = %lu\n", count, strlen(packets[count].data));
		if (count == 0)	printf("%s\n", packets[count].data);
		++count;
	}
#endif

	/* Step 1: Create the socket */
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("cannot create socket");
		close(sockfd);
		return 0;
	}
	
	// Make the recvfrom sockfd[1] able to timeout
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
		

	/* Step 3: Send filename and filesize to server using packet */
	//if ((bytes = sendto(sockfd, &packets[0], sizeof (packets[0]), 0, ((struct sockaddr *)&servaddr), alen)) == -1) {
	//	perror("sendto");
	//	close(sockfd);
	//	return 0;
	//}

	printf("\n-----------%s----------\n", filename);

//return 0;
	/* Step 4: Send packets to the server */
	count = 0;
		do {
			//printf("%lu\n", sizeof packets[count].data);
			Packet ack;
			/* Sending data to server using packets */
			if ((bytes = sendto (sockfd, &packets[count], sizeof packets[count], 0, (struct sockaddr *)&servaddr, alen)) == -1) {
				perror("sendto, client-side");
				close(sockfd);
				return 0;
			}

			//printf("Sent to client\n");
			/* TODO: add a time-out mechanism using pthreads */
			/* Waiting for ACK from server */
			if ((bytes = recvfrom (sockfd, &ack, sizeof ack, 0, (struct sockaddr *)&servaddr, &alen)) == -1) {
				printf("Did not receive ACK...trying again\n");
				continue;
				//perror("Did not receive ACK");
				//return 0;
			}
		//	printf("ack.hp.opcode==%d = 0 && ack.hp.sequenceNumber == %d == %d\n", ack.hp.opcode, ack.hp.sequenceNumber, packets[count].hp.sequenceNumber);
			/* Check if ACK packet contains same sequenceNumber */
			if (ack.hp.opcode == 0 && ack.hp.sequenceNumber == packets[count].hp.sequenceNumber) {
				++count; // Successfully sent out packet
			}
		//	printf("count=%d=%d\n",count, totalpackets);
		} while (count < totalpackets);

	Packet dummy;
	dummy.hp.opcode = 0;
	/* Step 5: Tell server to stop waiting for packets */
	if ((bytes = sendto(sockfd, &dummy, sizeof dummy, 0, ((struct sockaddr *)&servaddr), alen)) == -1) {
		perror("sendto");
		return 0;
	}
	
	close(sockfd); // Close the socket
	fclose(fptr);	// close the file

	return 0;
}
