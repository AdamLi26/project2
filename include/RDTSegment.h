#ifndef RDTSegment_INCLUDED
#define RDTSegment_INCLUDED

#include <string>
#include <cstdint>

/*

Total: 1024 bytes
--------------------------------------
|seqNum (2 bytes) | ackNum (2 bytes) |
--------------------------------------
|flag   (2 bytes) | recvW  (2 bytes) |
--------------------------------------
|data (1016 bytes)			         |
|						       	     |
|                                    |
--------------------------------------

For seqNum and ackNum: max number is 30,730

For flag
-----------------------------------------------------------------------
data length (10 bits) | ...(unsued 4 bits) |SYN (1 bit) | FIN (1 bit) |
-----------------------------------------------------------------------

*/

const unsigned int MAX_SEGMENT_SIZE = 1024;
const unsigned int MAX_SEQ_NUM = 30720;
const unsigned int TIMEOUT_MS = 500;

struct RDTHeader {
	uint16_t seqNum;
	uint16_t ackNum;
	uint16_t flag;
	uint16_t recvW;
};

struct RDTSegment {
	RDTHeader header;
	char data[1016];
};



// helpful functions
void toLocal(struct RDTHeader *h);
void toNetwork(struct RDTHeader *h);

bool isSyn(struct RDTHeader *h);
void setSyn(struct RDTHeader *h);
bool isFin(struct RDTHeader *h);
void setFin(struct RDTHeader *h);
uint16_t dataLength(struct RDTHeader *h);
void ack(struct RDTSegment *s);

void print(struct RDTSegment *s);


#endif
