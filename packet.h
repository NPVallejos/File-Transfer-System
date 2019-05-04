#define MAX 1500 - sizeof(Header)

/* opcode information:
	0 - ACK
	1 - WRQ
	2 - RRQ
*/
typedef struct {
	short opcode;
	short sequenceNumber;
} Header;

typedef struct {
	Header hp;
	char data[MAX];
} Packet;
