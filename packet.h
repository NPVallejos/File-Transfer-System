#define MAX 1500 - sizeof(Header) // 1500 is total packet size, so MAX is the size of the unsigned char data array

/* opcode information:
	0 - ACK
	1 - WRQ - write data to file
	2 - RRQ - read data 
*/
typedef struct {
	short opcode;
	int sequenceNumber; // TODO: Use this for go back n mechanism
} Header;

typedef struct {
	Header hp;
	unsigned char data[MAX];	// Hold file data here
} Packet;
