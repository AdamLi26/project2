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
------------------------------------------------       
..(unused 14 bits)  |SYN (1 bit) | FIN (1 bit) |
------------------------------------------------

*/

const unsigned int MAXSEGMENTSIZE = 1024;
const unsigned int MAXSEQNUM = 30720;

struct {
	uint16_t seqNum;
	uint16_t ackNum;
	uint16_t flag;
	uint16_t recvW;
} RDTHeader;

struct {
	RDTHeader header;
	char *data;
} RDTSegment;



// helpful functions
bool isSyn(struct RDTSegment *segment);
bool isFin(struct RDTSegment *segment);
void ack(struct RDTSegment *segment);


#endif