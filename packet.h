#define MAX 1500 - sizeof(Header)
/* opcode information:
	0 - ACK
	1 - WRQ
	2 - RRQ
*/
typedef struct {
	short opcode;
	int sequenceNumber;
} Header;

typedef struct {
	Header hp;
	unsigned char data[MAX];
} Packet;
