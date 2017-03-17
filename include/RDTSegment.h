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
		   (unsued 13 bits)  | ACK (1 bit) |SYN (1 bit) | FIN (1 bit) |
-----------------------------------------------------------------------

*/

const unsigned int MAX_SEGMENT_SIZE = 1024;
const unsigned int SEGMENT_PAYLOAD_SIZE = 1016;
const unsigned int MAX_SEQ_NUM = 30720;
const unsigned int TIMEOUT_MS = 500;
const unsigned int WINDOW_SIZE = 5120;

struct RDTHeader {
	uint16_t seqNum;
	uint16_t ackNum;
	uint16_t flag;
	uint16_t recvW;
};

struct RDTSegment {
	RDTHeader header;
	char data[SEGMENT_PAYLOAD_SIZE];
};


// helpful functions
bool isAck(struct RDTHeader *h);
bool isSyn(struct RDTHeader *h);
bool isFin(struct RDTHeader *h);
uint16_t generateAck(struct RDTSegment *s); /* generate the corresponding ACK number for segment s */

void setAck(struct RDTHeader *h);
void setSyn(struct RDTHeader *h);
void setFin(struct RDTHeader *h);

void toLocal(struct RDTHeader *h);
void toNetwork(struct RDTHeader *h);
void print(struct RDTSegment *s);


#endif
